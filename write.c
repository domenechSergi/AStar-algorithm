#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

typedef struct {
    unsigned long id;           
    unsigned short namelen;    
    char* name;                 
    double lat, lon;            
    unsigned short nsucc;       
    unsigned long* successors;  
} node;


void ExitError(const char *miss, int errcode) {
    fprintf (stderr, "\nERROR: %s.\nStopping...\n\n", miss); exit(errcode);
}


//Binary search function 
signed long binary_search(node* nodes, unsigned long id, unsigned long low, unsigned long high) {
    if ( nodes[low].id == id ) return (signed long)low;
    if ( nodes[high].id == id ) return (signed long)high;
    signed long index = (high + low)/2;
    unsigned long guess = nodes[index].id;
    while ( (high-low) > 1 ) {
        if (guess == id) { return index; }
        if (guess > id) { high = index; }
        if (guess < id) { low = index; }
        index = (high + low)/2;
        guess = nodes[index].id;
    }
    return -1; 
}


//Updates information (total number of successors)
void update_nsuccs(unsigned short* nsuccdim, char* line, node* nodes, unsigned long nnodes) {
    bool oneway = false;                            
    char* field = strsep(&line, "|");              
    if (*field != 'w') return;                      
    unsigned short count;
    for (count = 2; count < 9; count++) field = strsep(&line, "|");                        
    if (*field == 'o') oneway = true;                
    field = strsep(&line, "|");                      
    
    char* ptr = NULL;                                
    char* node_n = NULL;                             
    char* node_m = NULL;                             
    unsigned long id_n, id_m;                        
    signed long n = -1, m;                           
    while ((n == -1) && ((node_n = strsep(&line, "|")) != NULL)) { 
        id_n = strtoul(node_n, &ptr, 10);
        n = binary_search(nodes, id_n, 0, nnodes-1);
    }
    while ((node_m = strsep(&line, "|")) != NULL) {  
        id_m = strtoul(node_m, &ptr, 10);
        m = binary_search(nodes, id_m, 0, nnodes-1);
        if (m == -1) continue;                       
        if (oneway == true) nsuccdim[n] += 1;
        else { nsuccdim[n] += 1; nsuccdim[m] += 1; }
        n = m;                                       
    }
}


//Updates information (adjacent nodes)
void update_successors(char* line, node* nodes, unsigned long nnodes, unsigned short* counters) {
    bool oneway = false;                            
    char* field = strsep(&line, "|");               
    if (*field != 'w') return;                      
    unsigned short count;
    for (count = 2; count < 9; count++) field = strsep(&line, "|");                      
    if (*field == 'o') oneway = true;                
    field = strsep(&line, "|");                      
    
    char* ptr = NULL;                                
    char* node_n = NULL;                             
    char* node_m = NULL;                             
    unsigned long id_n, id_m;                        
    signed long n = -1, m;                           
    while ((n == -1) && ((node_n = strsep(&line, "|")) != NULL)) { 
        id_n = strtoul(node_n, &ptr, 10);
        n = binary_search(nodes, id_n, 0, nnodes-1);
    }
    unsigned short free_n, free_m;                   
    while ((node_m = strsep(&line, "|")) != NULL) {  
        id_m = strtoul(node_m, &ptr, 10);
        m = binary_search(nodes, id_m, 0, nnodes-1);
        if (m == -1) continue;                       
        free_n = counters[n];                         
        free_m = counters[m];                         
        if (oneway == true) { (nodes[n].successors)[free_n] = m;  counters[n] += 1; } 
        else {                                                                        
            (nodes[n].successors)[free_n] = m;    counters[n] += 1;
            (nodes[m].successors)[free_m] = n;    counters[m] += 1;
        }
        n = m;                                        
    }
}

//Updates information (node)
void get_node(node* nodes, char* line, unsigned long i) {
    char* field = NULL;
    field = strsep(&line, "|");                                             
    if (*field != 'n') return;                                               
    field = strsep(&line, "|");                                              
    char* ptr = NULL;                                                        
    nodes[i].id = strtoul(field, &ptr, 10);
    field = strsep(&line, "|");                                              
    nodes[i].namelen = strlen(field);
    if ((nodes[i].name = (char*) malloc(nodes[i].namelen + 1)) == NULL) ExitError("when allocating memory for a node name", 1);
    strcpy( nodes[i].name, field );
    unsigned short count;
    for (count = 4; count < 11; count++) field = strsep(&line, "|");         
    nodes[i].lat = atof(field);
    field = strsep(&line, "|");                                              
    nodes[i].lon = atof(field);
}



int main (int argc, char *argv[]) {
    
   FILE *fmap;
    if ((fmap = fopen(argv[1], "r")) == NULL) ExitError("when opening the csv file", 2);
    

    char* line_buf = NULL;      
    size_t line_buf_size = 0;   
    ssize_t line_size = 0;      
    
    unsigned long nnodes = 0UL;
    while (line_size >= 0) {
        line_size = getline(&line_buf, &line_buf_size, fmap);
        if (*(line_buf) == '#') continue;                    
        else if (*(line_buf) == 'n') nnodes += 1;           
        else if (*(line_buf) == 'w') break;                               
    }
    
    node *nodes;
    if((nodes = (node *) malloc(nnodes*sizeof(node))) == NULL) ExitError("when allocating memory for the nodes vector", 3);
    unsigned long i = 0;  
    unsigned short* nsuccdim = NULL;
    if ((nsuccdim = (unsigned short*) calloc(nnodes, sizeof(unsigned short))) == NULL) ExitError("when allocating memory for nsuccdim", 4);
        
    fseek(fmap, 0, SEEK_SET);                                
    while (line_size >= 0) {
        line_size = getline(&line_buf, &line_buf_size, fmap);
        if (*(line_buf) == '#') continue;                
        else if (*(line_buf) == 'n') {                   
            get_node(nodes, line_buf, i);                
            i += 1;
            continue;
        }
        else if (*(line_buf) == 'w') {                        
            update_nsuccs(nsuccdim, line_buf, nodes, nnodes); 
            continue;
        }
        else if (*(line_buf) == 'r') break;                                   
    }
    
    for (i = 0; i < nnodes; i++) {
        nodes[i].nsucc = nsuccdim[i];
        if((nodes[i].successors = (unsigned long*) malloc(nsuccdim[i]*sizeof(unsigned long))) == NULL) ExitError("when allocating memory for the nodes successors", 5);
    }
    
    unsigned short* succ_counter = NULL;
    if((succ_counter = (unsigned short*) calloc(nnodes, sizeof(unsigned short))) == NULL) ExitError("when allocating memory for the nodes successor counters", 6);
    
    fseek(fmap, 0, SEEK_SET);                                      
    while (line_size >= 0) {
        line_size = getline(&line_buf, &line_buf_size, fmap);      
        if (*(line_buf) == '#' || *(line_buf) == 'n') continue;    
        if (*(line_buf) == 'w') {                                  
            update_successors(line_buf, nodes, nnodes, succ_counter);                                                   
        }
        if (*(line_buf) == 'r') break;                              
    }
    free(succ_counter);     
    free(line_buf);         
    fclose(fmap);          
    
    FILE *fin;
    char name[257];
    
    unsigned long ntotnsucc = 0UL;
    unsigned long totnamelen = 0UL;
    for(i = 0; i < nnodes; i++) {
        ntotnsucc += nodes[i].nsucc;
        totnamelen += nodes[i].namelen;
    }
    totnamelen += 1;
    
    char* allnames;
    if((allnames = (char*) malloc((totnamelen)*sizeof(char))) == NULL) ExitError("when allocating memory for allnames vector", 7);
    
    strcpy(name, argv[1]); strcpy(strrchr(name, '.'), ".bin");
    if ((fin = fopen (name, "wb")) == NULL)
        ExitError("the output binary data file cannot be opened", 8);
    
    if( fwrite(&nnodes, sizeof(unsigned long), 1, fin) +
        fwrite(&ntotnsucc, sizeof(unsigned long), 1, fin) +
        fwrite(&totnamelen, sizeof(unsigned long), 1, fin) != 3 )
            ExitError("when initializing the output binary data file", 9);

    if( fwrite(nodes, sizeof(node), nnodes, fin) != nnodes )
        ExitError("when writing nodes to the output binary data file", 10);
       
    for(i = 0; i < nnodes; i++){
        if(nodes[i].nsucc) {
            if ( fwrite(nodes[i].successors, sizeof(unsigned long), nodes[i].nsucc, fin) != nodes[i].nsucc )
                ExitError("when writing edges to the output binary data file", 11);
        }
        strcat(allnames, nodes[i].name);
    }
    
    if ( fwrite(allnames, sizeof(char), totnamelen, fin) != totnamelen )
            ExitError("when writing allnames to the output binary data file", 12);
        
    fclose(fin);            
                  
    for (i = 0; i < nnodes; i++) { free(nodes[i].name); free(nodes[i].successors); }
    free(nodes);
    free(nsuccdim);
    free(allnames);
    
    return 0;
}





