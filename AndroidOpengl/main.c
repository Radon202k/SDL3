#include "platform.h"

int
main(int argc, char **argv)
{
    Platform plat = platform_init();
    
    while (!plat.quit)
    {
        plat_frame_begin(&plat);
        
        // Bind
        glUseProgram(plat.testShader.program);
        glBindVertexArray(plat.testBuffers.vao);
        glDrawArrays(GL_TRIANGLES,
                     0,
                     3);
        
        plat_frame_end(&plat);
    }
    
    platform_shutdown(&plat);
    
    return 0;
}