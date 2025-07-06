#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {int x,y;} Cell;

static int rand_int(int max) { return rand() % max; }

int main(int argc, char **argv) {
    if(argc < 3) {
        printf("Usage: %s <width> <height>\n", argv[0]);
        return 1;
    }
    int w = atoi(argv[1]);
    int h = atoi(argv[2]);
    if(w <=0 || h <=0) {
        printf("Width and height must be positive.\n");
        return 1;
    }
    int gw = w * 2 + 1;
    int gh = h * 2 + 1;
    char *grid = (char*)malloc(gw*gh);
    if(!grid) return 1;
    for(int i=0;i<gh;i++)
        for(int j=0;j<gw;j++)
            grid[i*gw + j] = '#';
    int *visited = (int*)calloc(w*h, sizeof(int));
    Cell *stack = (Cell*)malloc(w*h*sizeof(Cell));
    int sp = 0;
    srand((unsigned)time(NULL));
    stack[sp++] = (Cell){0,0};
    visited[0]=1;
    grid[1*gw + 1] = ' ';
    while(sp) {
        Cell cur = stack[sp - 1];
        int dirs[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
        int opts[4];
        int optc=0;
        for(int d=0;d<4;d++) {
            int nx=cur.x+dirs[d][0];
            int ny=cur.y+dirs[d][1];
            if(nx>=0 && nx<w && ny>=0 && ny<h && !visited[ny*w+nx])
                opts[optc++] = d;
        }
        if(optc>0) {
            int choice = opts[rand_int(optc)];
            int nx=cur.x+dirs[choice][0];
            int ny=cur.y+dirs[choice][1];
            int cx = cur.x * 2 + 1;
            int cy = cur.y * 2 + 1;
            grid[(cy + dirs[choice][1]) * gw + cx + dirs[choice][0]] = ' ';
            grid[(ny * 2 + 1) * gw + (nx * 2 + 1)] = ' ';
            visited[ny*w+nx]=1;
            stack[sp++] = (Cell){nx,ny};
        } else {
            sp--;
        }
    }
    for(int i=0;i<gh;i++) {
        for(int j=0;j<gw;j++) {
            putchar(grid[i*gw+j]);
        }
        putchar('\n');
    }
    free(grid); free(visited); free(stack);
    return 0;
}

