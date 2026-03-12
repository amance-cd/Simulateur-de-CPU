#include "cpu.h"

int main(int argc, char *argv[]){

    if (argc != 2) {
        printf("Usage: %s <fichier.asm>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    ParserResult *result = parse(filename);

    resolve_constants(result);

    if(!result){
        return 1;
    }

    int memory_size = 1024;
    CPU* cpu = cpu_init(memory_size);

    allocate_variables(cpu,result->data_instructions,result->data_count);
    allocate_code_segment(cpu,result->code_instructions,result->code_count);

    free_parser_result(result);

    run_program(cpu);

    cpu_destroy(cpu);

}