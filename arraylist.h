typedef struct {
    char **data;
    int length;
    int capacity;
} arraylist_t;


void al_init(arraylist_t *, int);
void al_destroy(arraylist_t *);
unsigned al_length(arraylist_t *);
void al_push(arraylist_t *, char *);
int al_pop(arraylist_t *, char *);


