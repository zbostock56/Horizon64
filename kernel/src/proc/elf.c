/**
 * @file elf.c
 * @author Zack Bostock
 * @brief Functionality for loading and running Executable and Linkable Files (ELF)
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <proc/elf.h>
#include <proc/process.h>

#include <fs/vfs.h>

#include <common/string.h>
#include <common/math.h>

#define RTDL_ADDR       (0x40000000)
#define IS_TEXT(p)      (p.flags & PF_X)
#define IS_DATA(p)      (p.flags & PF_W)
#define IS_BSS(p)       (p.filesz < p.memsz)

extern LOCK ctxsw_lock;

/**
 * @brief Helper for findind the symbol table
 *
 * @param header ELF header
 * @param sheader ELF Symbol Header
 * @return size_t Location of the symbol table
 */
size_t elf_find_symbol_table(ELF_HEADER *header, ELF_SHEADER *sheader) {
    for (size_t i = 0; i < header->shnum; i++) {
        if (sheader[i].type == SHT_SYMTAB) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Helper for finding a symbol in the symbol table
 *
 * @param name Name of the symbol
 * @param sheader Symbol header
 * @param sym Symbol to find
 * @param src Source of the symbol
 * @param dst Destination of the symbol
 * @return void* Pointer to the symbol
 */
void *elf_find_symbol(const char *name, ELF_SHEADER *sheader, ELF_SHEADER *sym,
                      const char *src, char *dst) {
    ELF_SYM *syms = (ELF_SYM *)(src + sym->offset);
    const char *strings = src + sheader[sym->link].offset;

    for (size_t i = 0; i < sym->size / sizeof(ELF_SYM); i++) {
        if (!strcmp(name, strings + sym[i].name)) {
            return dst + syms[i].value;
        }
    }

    return NULL;
}

/**
 * @brief Loads and ELF executable
 *
 * @param p Process to load
 * @param path_name Path to executable
 * @param entry_point Entry point of the executable
 * @param aux ELF AUXVAl
 * @return size_t 0 if success, -1 if fail
 */
size_t elf_load(PROCESS *p, const char *path_name, uint64_t *entry_point,
                ELF_AUXVAL *aux) {
    klogd("%s: Loading elf binary at %s for process %d\n", __func__,
          path_name, p->id);
    uint8_t *elf_buff = NULL;
    size_t elf_len = 0;

    int has_dyn_linking = FALSE;
    ELF_AUXVAL dyn_aux = {0};

    ELF_PHEADER *pheader = NULL;
    ELF_SHEADER *sheader = NULL;
    uint64_t *phaddr = NULL;

    MEM_MAP m;
    m.flags = VM_DEFAULT | VM_USERMODE;

    /* Open up the executable and read the header */
    const char *fn = path_name;
    VFS_HANDLE f = vfs_open((char *)fn, VFS_READ);
    if (f != VFS_INVALID_HANDLE) {
        elf_len = vfs_tell(f);
        elf_buff = (uint8_t *)(kmalloc(elf_len));
        if (!elf_buff) {
            kloge("ELF LOAD: Failed to allocate memory!\n");
            goto error;
        } else {
            size_t readlen = vfs_read(f, elf_len, elf_buff);
            if (!readlen) {
                kloge("VFS: opened \"%s\" successfully but cannot read data!\n", fn);
            } else {
                klogd("ELF Load: (%s) read %d bytes\n", path_name, readlen);
            }

            m.virt_addr = (uint64_t)(elf_buff);
            m.phys_addr = VIRT_TO_PHYS(elf_buff);
            m.num_pages = NUM_PAGES(elf_len);

            vector_append(&p->memmap_list, m);
        }
        vfs_close(f);
    } else {
        kloge("VFS: open \"%s\" failed!\n", fn);
    }

    if (!elf_buff) {
        goto error;
    }

    klogd("%s: successfully opened elf binary\n", __func__);

    ELF_HEADER h = {0};
    memcpy(&h, elf_buff, sizeof(ELF_HEADER));

    /* Check to make sure that the ELF header is good */
    if (h.magic != ELF_MAGIC || h.elf[EI_CLASS] != 0x2 ||
        h.elf[EI_DATA] != BITS_LE || h.elf[EI_OSABI] != ABI_SYSV ||
        h.machine != ARCH_X86_64) {
        goto error;
    }

    klogd("%s: ELF binary header is good\n", __func__);

    aux->entry = h.entry;
    if (h.type == ET_SHARED) {
        aux->entry += RTDL_ADDR;
    }
    aux->phdr = 0;
    aux->phnum = h.phnum;
    aux->phentsize = h.phentsize;

    pheader = kmalloc(h.phnum * sizeof(ELF_PHEADER));
    if (!pheader) {
        kloge("ELF LOAD: Failed to allocate memory for pheader!\n");
        goto error;
    }
    memcpy(pheader, elf_buff + h.phoff, h.phnum * sizeof(ELF_PHEADER));

    m.virt_addr = (uint64_t)pheader;
    m.phys_addr = VIRT_TO_PHYS(pheader);
    m.num_pages = NUM_PAGES(h.phnum * sizeof(ELF_PHEADER));

    vector_append(&p->memmap_list, m);

    phaddr = (uint64_t *)(kmalloc(h.phnum * sizeof(uint64_t)));
    if (!phaddr) {
        kloge("ELF LOAD: Failed to allocate memory phaddr!\n");
        goto error;
    }
    aux->phaddr = (uint64_t)phaddr;

    m.virt_addr = (uint64_t)phaddr;
    m.phys_addr = VIRT_TO_PHYS(phaddr);
    m.num_pages = NUM_PAGES(h.phnum * sizeof(uint64_t));

    vector_append(&p->memmap_list, m);

    for (size_t i = 0; i < h.phnum; i++) {
        phaddr[i] = (uint64_t)NULL;

        if (pheader[i].type == PT_INTERP && pheader[i].filesz > 0 &&
            !has_dyn_linking) {
            char *rdtl_path = (char *)(kmalloc(pheader[i].filesz + 1));
            if (rdtl_path) {
                memcpy(rdtl_path, &(elf_buff[pheader[i].offset]),
                       pheader[i].filesz);
                rdtl_path[pheader[i].filesz] = '\0';
                has_dyn_linking = TRUE;
                elf_load(p, rdtl_path, NULL, &dyn_aux);
                kfree(rdtl_path);
            } else {
                kloge("ELF LOAD: Failed to allocate memory for rdtl_path!\n");
                goto error;
            }
            continue;
        }

        if (pheader[i].type == PT_PHDR) {
            aux->phdr = pheader[i].vaddr;
            if (h.type == ET_SHARED) {
                aux->phdr += RTDL_ADDR;
            }
            continue;
        }

        if (pheader[i].type != PT_LOAD) {
            continue;
        }

        size_t misalign = pheader[i].vaddr & (PAGE_SIZE - 1);
        size_t page_count = DIV_ROUNDUP(misalign + pheader[i].memsz, PAGE_SIZE);

        uint64_t addr = VIRT_TO_PHYS(kmalloc(page_count * PAGE_SIZE));
        if (!addr) {
            kloge("ELF LOAD: Failed to allocate memory!\n");
            goto error;
        }

        phaddr[i] = addr;

        size_t pf = VM_DEFAULT | VM_USERMODE;
        if (pheader[i].flags & PF_W) {
            pf |= VM_READ_WRITE;
        }

        uint64_t virt = pheader[i].vaddr - misalign;
        if (h.type == ET_SHARED) {
            virt += RTDL_ADDR;
        }

        vm_map(p->addrspace, virt, addr, page_count, pf);

        memset((void *)PHYS_TO_VIRT(addr), 0, PAGE_SIZE * page_count);

        MEM_MAP m1;
        m1.virt_addr = virt;
        m1.phys_addr = addr;
        m1.num_pages = page_count;
        m1.flags = pf;

        vector_append(&p->memmap_list, m1);

        memcpy((void *)PHYS_TO_VIRT(addr + misalign), elf_buff + pheader[i].offset,
                pheader[i].filesz);
    }

    sheader = kmalloc(h.shnum * sizeof(ELF_SHEADER));
    if (!sheader) {
        kloge("ELF OPEN: Failed to allocate memory for sheader!\n");
        goto error;
    }

    memset(sheader, 0, h.shnum * sizeof(ELF_SHEADER));
    m.virt_addr = (uint64_t)sheader;
    m.phys_addr = VIRT_TO_PHYS(sheader);
    m.num_pages = NUM_PAGES(h.shnum * sizeof(ELF_SHEADER));

    vector_append(&p->memmap_list, m);

    aux->shdr = (uint64_t)sheader;
    memcpy(sheader, elf_buff + h.shoff, h.shnum * sizeof(ELF_SHEADER));

    if (has_dyn_linking) {
        if (entry_point) {
            *entry_point = dyn_aux.entry;
        }
    } else {
        if (entry_point) {
            *entry_point = aux->entry;
        }
    }

    /* Have to free pheader, phaddr, sheader, and elf_buff when process dies */
    return 0;


    error:
        kloge("ELF LOAD: (%s) File header error\n", path_name);
        if (phaddr) {
            kfree(phaddr);
        }
        if (pheader) {
            kfree(pheader);
        }
        if (sheader) {
            kfree(sheader);
        }
        if (elf_buff) {
            kfree(elf_buff);
        }
        return -1;
}