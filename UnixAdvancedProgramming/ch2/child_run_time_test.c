#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main ()
{
    int i ;
    int j ;

    for (i = 0 ; i < 1000 ; i ++ )
        for (j = 0 ; j < 1000 ; j ++ )
            open ("Cannon-1.txt" , O_RDONLY );

    return (0 );
}
