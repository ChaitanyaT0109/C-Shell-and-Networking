#include "executor.h"
#include "commands.h"
#include "background.h"
#include "pipes.h"
#include "redirection.h"

int execute_command(char** args) {
    if (args[0] == NULL) {
        // Empty command
        return 1;
    }
    
    // Parse and execute as pipeline
    pipeline_t pipeline;
    parse_pipeline(args, &pipeline);
    
    int result = execute_pipeline(&pipeline);
    
    cleanup_pipeline(&pipeline);
    return result;
}
