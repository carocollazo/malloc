#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "mminline.h"
#include "mm.h"

#define USAGE                                                            \
    "./run_tests <all | "                                                \
    "names of tests to run>"                                             \
    "\n   Ex. \"./inline_tests all\" runs all tests"                        \
    "\n   Ex. \"./inline_tests set_flink set_blink\" runs the set_flink and set_blink " \
    "\n   Ex. \"./inline_tests pull_free_block\" runs the pull_free_block test" \
    "\n   Possible tests: 'set_flink', 'set_blink', 'pull_free_block'"

void assert_flink(block_t *expected, block_t *actual, const char *message);

void assert_blink(block_t *expected, block_t *actual, const char *message);

void assert_pull_free_block(block_t *expected, block_t *actual, const char *message);

void print_test_summary();

static block_t *flist_first;
block_t* prologue;
block_t* epilogue;

void set_flink_test() {
    prologue = malloc(16);
    epilogue = malloc(16);
    block_t *cur_block = (block_t *)malloc(8*2 + 32);
    block_t *new_flink = (block_t *)malloc(8*2 + 64);
    block_set_size_and_allocated(cur_block, 32, 0);
    block_set_size_and_allocated(new_flink, 64, 0);
    block_set_flink(cur_block, new_flink);

    assert(block_flink(cur_block) == new_flink);

    assert(block_size(cur_block) == 32);
    assert(block_size(new_flink) == 64);
    assert(!block_allocated(cur_block));
    assert(!block_allocated(new_flink));
    free(prologue);
    free(epilogue);
    free(cur_block);
    free(new_flink);
}

void set_blink_test() {
    prologue = malloc(16);
    epilogue = malloc(16);
    block_t *cur_block = (block_t *)malloc(8*2 + 32);
    block_t *new_blink = (block_t *)malloc(8*2 + 64);
    block_set_size_and_allocated(cur_block, 32, 0);
    block_set_size_and_allocated(new_blink, 64, 0);
    block_set_blink(cur_block, new_blink);
    
    assert(block_blink(cur_block) == new_blink);

    assert(block_size(cur_block) == 32);
    assert(block_size(new_blink) == 64);
    assert(!block_allocated(cur_block));
    assert(!block_allocated(new_blink));
    free(prologue);
    free(epilogue);
    free(cur_block);
    free(new_blink);
}

void pull_free_block_test() {
    prologue = malloc(16);
    epilogue = malloc(16);
    block_t *block_one = (block_t *)malloc(8*2 + 32);
    block_t *block_two = (block_t *)malloc(8*2 + 64);
    block_t *block_three = (block_t *)malloc(8*2 + 48);
    flist_first = NULL;
    block_set_size_and_allocated(block_one, 32, 0);
    block_set_size_and_allocated(block_two, 64, 0);
    block_set_size_and_allocated(block_three, 48, 0);
    insert_free_block(block_three);
    insert_free_block(block_two);
    insert_free_block(block_one);
    pull_free_block(block_two);
    
    sleep(1);
    assert(block_one->payload[0] == (int)((char *)block_three - (char *)prologue));
    assert(block_one->payload[1] == (int)((char *)block_three - (char *)prologue));
    assert(block_three->payload[0] == (int)((char *)block_one - (char *)prologue));
    assert(block_three->payload[1] == (int)((char *)block_one - (char *)prologue));
    assert(flist_first != NULL);
    assert((block_t *)flist_first == (block_t *) block_one);
    
    pull_free_block(block_three);
    sleep(1);
    assert(block_one->payload[0] == (int)((char *)block_one - (char *)prologue));
    assert(block_one->payload[1] == (int)((char *)block_one - (char *)prologue));
    assert(flist_first != NULL);
    assert((block_t *)flist_first == (block_t *) block_one);

    pull_free_block(block_one);
    sleep(1);
    assert(flist_first == NULL);

    block_t *block_four = (block_t *)malloc(8*2 + 32);
    block_t *block_five = (block_t *)malloc(8*2 + 64);
    block_t *block_six = (block_t *)malloc(8*2 + 48);
    block_t *block_seven = (block_t *)malloc(8*2 + 96);
    block_set_size_and_allocated(block_four, 32, 0);
    block_set_size_and_allocated(block_five, 64, 0);
    block_set_size_and_allocated(block_six, 48, 0);
    block_set_size_and_allocated(block_seven, 96, 0);

    insert_free_block(block_seven);
    insert_free_block(block_six);
    insert_free_block(block_five);
    insert_free_block(block_four);

    pull_free_block(block_four);
    
    sleep(1);
    assert(block_five->payload[0] == (int)((char *)block_six - (char *)prologue));
    assert(block_five->payload[1] == (int)((char *)block_seven - (char *)prologue));
    
    assert(block_seven->payload[0] == (int)((char *)block_five - (char *)prologue));
    assert(block_seven->payload[1] == (int)((char *)block_six - (char *)prologue));
    assert(flist_first != NULL);
    assert((block_t *)flist_first == (block_t *) block_five);

    pull_free_block(block_five);
    sleep(1);
    assert(block_six->payload[1] == (int)((char *)block_seven - (char *)prologue));
    assert(block_six->payload[0] == (int)((char *)block_seven - (char *)prologue));

    assert(block_seven->payload[1] == (int)((char *)block_six - (char *)prologue));
    assert(block_seven->payload[0] == (int)((char *)block_six - (char *)prologue));
    assert(flist_first != NULL);
    assert((block_t *)flist_first == (block_t *) block_six);

    pull_free_block(block_six);
    sleep(1);
    assert(block_seven->payload[1] == (int)((char *)block_seven - (char *)prologue));
    assert(block_seven->payload[0] == (int)((char *)block_seven - (char *)prologue));
    assert(flist_first != NULL);
    assert((block_t *)flist_first == (block_t *) block_seven);
     
    pull_free_block(block_seven);
    sleep(1);
    assert(flist_first == NULL);

    free(prologue);
    free(epilogue);
    free(block_one);
    free(block_two);
    free(block_three);
    free(block_four);
    free(block_five);
    free(block_six);
    free(block_seven);
}

int total_tests, num_correct, num_incorrect;
int run_test_in_separate_process(void (*func)(), int num_tests, const char *message) {
    printf("running test: ");
    printf("%s\n", message);
    func();
    num_tests--;
    return 0;
}

void foreach_test(int num_tests, char const *test_names[],
                  int (*wrapper)(void (*)(), int, const char *)){
    int functions_passed = 0;
    int dummy;
    if (!strcmp(test_names[1], "all")){
        functions_passed += wrapper(&set_blink_test,5, "set_blink");
        functions_passed += wrapper(&set_flink_test, 6, "set_flink");
        functions_passed += wrapper(&pull_free_block_test, 4, "pull_free_block");
        return;
    }

    for (int i = 0; i < num_tests; ++i){
        if (i == 0)
            continue; // file name

        const char *test_name = test_names[i];
        if (!strcmp(test_name, "set_blink"))
            functions_passed += wrapper(&set_blink_test,5, "set_blink");
        else if (!strcmp(test_name, "set_flink"))
            functions_passed += wrapper(&set_flink_test, 6, "set_flink");
        else if (!strcmp(test_name, "pull_free_block"))
            functions_passed += wrapper(&pull_free_block_test, 4, "pull_free_block");
        else if (sscanf(test_name, "%d", &dummy) != 1)
            printf("Unknown test: %s\n", test_name);
    }
}

int main(int argc, char const *argv[]){
    printf("Testing for correctness...\n");

    if (argc < 2){
        printf("USAGE: %s\n", USAGE);
        return 1;
    }

    foreach_test(argc, argv, &run_test_in_separate_process);

    printf("Passed all tests!\n");

    return 0;
}
