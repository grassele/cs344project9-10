#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MEM_SIZE 16384  // MUST equal PAGE_SIZE * PAGE_COUNT
#define PAGE_SIZE 256  // MUST equal 2^PAGE_SHIFT
#define PAGE_COUNT 64
#define PAGE_SHIFT 8  // Shift page number this much

#define PTP_OFFSET 64 // How far offset in page 0 is the page table pointer table


unsigned char mem[MEM_SIZE]; // simulated RAM


int get_address(int page, int offset) {
    return (page << PAGE_SHIFT) | offset;
}


int get_physical_address(proc_num, virt_addr) {
    int ptp_num = mem[proc_num + PTP_OFFSET];
    int ptp_addr = get_address(ptp_num, 0);

    int virt_page = virt_addr >> 8;
    int offset = virt_addr & 255;

    int phys_page = mem[ptp_addr + virt_page];
    return (phys_page << PAGE_SHIFT) | offset;
}


void initialize_mem(void) {
    memset(mem, 0, MEM_SIZE);

    int zpfree_addr = get_address(0, 0);
    mem[zpfree_addr] = 1;  // Mark zero page as allocated
}


unsigned char get_page_table(int proc_num) {
    int ptp_addr = get_address(0, PTP_OFFSET + proc_num);
    return mem[ptp_addr];
}


int allocate_page() {
    for (int i = 1; i < 64; i++) {
        if (mem[i] != 1) {
            mem[i] = 1;
            return i;
        }
    }
    return 0xff; // indicating no free pages
}


void deallocate_page(int p) {
    if ((0 <= p) && (p < 64)) {
        mem[p] = 0;
    }
}


void new_process(int proc_num, int page_count) {
    int page_table = allocate_page();

    if (page_table == 0xff) {
        printf("OOM: proc %d: page table\n", proc_num);
        return;
    }

    else {
        mem[64 + proc_num] = page_table;

        for (int i = 0; i < page_count; i++) {
            int new_page = allocate_page();

            if (new_page == 0xff) {
                printf("OOM: proc %d: data page\n", proc_num);
            }   
            else {
                int pt_addr = get_address(page_table, i);
                mem[pt_addr] = new_page;
            }
        }
    }
}


void kill_process(int p) {
    if ((0 <= p) && (p < 192)) {
        int page_table_page = mem[p + PTP_OFFSET];
        
        for (int offset = 0; offset <= 255; offset++) {
            int page_address = get_address(page_table_page, offset);
            if (mem[page_address] != 0) {
                deallocate_page(mem[page_address]);
            }
        }
        deallocate_page(page_table_page);
    }
    else {
        printf("Invalid process number. Must be 0-63\n");
    }
}


void store_value(proc_num, virt_addr, value) {
    int phys_addr = get_physical_address(proc_num, virt_addr);
    mem[phys_addr] = value;
    printf("Store proc %d: %d => %d, value=%d\n", proc_num, virt_addr, phys_addr, value);
}


void load_value(proc_num, virt_addr) {
    int phys_addr = get_physical_address(proc_num, virt_addr);
    int value = mem[phys_addr];
    printf("Load proc %d: %d => %d, value=%d\n", proc_num, virt_addr, phys_addr, value);
}


void print_page_free_map(void) {
    printf("--- PAGE FREE MAP ---\n");

    for (int i = 0; i < 64; i++) {
        int addr = get_address(0, i);

        printf("%c", mem[addr] == 0? '.': '#');

        if ((i + 1) % 16 == 0)
            putchar('\n');
    }  
}


void print_page_table(int proc_num) {
    printf("--- PROCESS %d PAGE TABLE ---\n", proc_num);

    int page_table = get_page_table(proc_num);

    for (int i = 0; i < PAGE_COUNT; i++) {
        int addr = get_address(page_table, i);

        int page = mem[addr];

        if (page != 0) {
            printf("%02x -> %02x\n", i, page);
        }
    }
}


int main(int argc, char *argv[]) {
    assert(PAGE_COUNT * PAGE_SIZE == MEM_SIZE);

    if (argc == 1) {
        fprintf(stderr, "usage: ptsim commands\n");
        return 1;
    }
    
    initialize_mem();

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "pfm") == 0) {
            print_page_free_map();
        }
        else if (strcmp(argv[i], "ppt") == 0) {
            int proc_num = atoi(argv[++i]);
            print_page_table(proc_num);
        }
        else if (strcmp(argv[i], "np") == 0) {
            new_process(atoi(argv[i+1]), atoi(argv[i+2]));
            i += 2;
        }
        else if (strcmp(argv[i], "kp") == 0) {
            kill_process(atoi(argv[++i]));
        }
        else if (strcmp(argv[i], "sb") == 0) {
            int proc_num = atoi(argv[++i]);
            int virt_addr = atoi(argv[++i]);
            int value = atoi(argv[++i]);
            store_value(proc_num, virt_addr, value);
        }
        else if (strcmp(argv[i], "lb") == 0) {
            int proc_num = atoi(argv[++i]);
            int virt_addr = atoi(argv[++i]);
            load_value(proc_num, virt_addr);
        }      
        else {
            fprintf(stderr, "usage: ptsim commands, \'%s\' not recognized\n", argv[i]);
            return 1;
        }
    } 
}
