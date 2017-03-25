#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


int nframes = 0; /* number of frames */
int algo = -1;  /* algorithm that is chosen */
int hand = 0;	/*clock pointer for CLOCK */
int pt = 0; /*pointer for VMS*/
int clean_pt = 0; /* pointer for clean list */
int dirty_pt = 0; /* pointer for dirty list */
int replace = 0; 	/* replace index */

int table_length = 0; /*table length for inserting page and CLOCK */

FILE *fp = 0;
FILE *fp2 = 0; /* Copy of file pointer for tracefile reading for OPT algorithm */
long fp_position;

unsigned long long read_count = 0, /* read count */
		write_count = 0, 			/* write count */
		event_count = 0;			/* event count */



typedef unsigned addr;	/* virtual address */

typedef unsigned vpn;	/* virtual page number */

typedef struct page
{
    addr vpn; 			/* virtual page number */
    char rw;    			/* read or write */
    char dirty;				/* dirty bit */
    unsigned long long usedtime;	/* time this page was recently used (LRU) */
    int clock;	  			/* clock bit (CLOCK) */
    unsigned long long opt; 	/* time this page was referred in future sequence (OPT) */
} page_t;

/* Initiate a pagetable */
page_t *pagetable_init(int nframe);

/* Print a page */
void page_print(page_t page);

/* Print a pagetable */
void pagetable_print(page_t *pagetable, int length);

/* Find if a page is in pagetable */
int page_find(addr vpn, page_t *pagetable);

/* Insert a page to pagetable */
int page_insert(addr vpn, char rw, page_t *pagetable, int position);

/* Get a page info from pagetable */
page_t *page_get(page_t *pagetable, int position);

/* Print the final statistics to screen */
int stat_print(int nframes, unsigned long long event_count ,
		unsigned long long read_count,
		unsigned long long write_count );

/********************************************************/
/*page repalcement algorithms vms(), lru(), clk(), opt()*/
/********************************************************/
int vms(page_t *pagetable, page_t *clist, page_t *dlist, addr vpn, char rw);
/* except vms, other algorithms return position of a page to replace */
int lru(page_t *pagetable);
int clk(page_t *pagetable);
int opt(page_t *pagetable);
/*****************************************/
int fifo(page_t *pagetable, int nframes, int pt);/*used by vms */
/*****************************************/

/********************************************************/
/********************************************************/
/********************************************************/
int main(int argc, char *argv[]) {

/* Simulator interface
 * Similator memsim takes the following arguments
 * memsim <tracefile> <nframes> <vms|lru|clk|opt> <debug|quiet>
 * */

	if(argc != 5) {
		printf("USAGE: memsim <tracefile> <nframes> <vms|lru|clk|opt> <debug|quiet>\n");
		return 1;
	}

	/* get trace filce name */
	char *tracefile = argv[1];

	/* get number of frames */
	nframes = atoi(argv[2]);
	if(nframes < 1){
		printf("USAGE: memsim <tracefile> <nframes> <vms|lru|clk|opt> <debug|quiet>\n");
		return 1;
	}

	/* get algorithm type */
	algo = -1;
	if(!strcmp(argv[3],"vms"))
		algo = 0;
	else if(!strcmp(argv[3],"lru"))
		algo = 1;
	else if(!strcmp(argv[3],"clk"))
		algo = 2;
	else if(!strcmp(argv[3],"opt"))
		algo = 3;
	else {
		printf("USAGE: memsim <tracefile> <nframes> <vms|lru|clk|opt> <debug|quiet>\n");
		return 1;
	}

	/* get debug option */
	int dq = -1;
	if(!strcmp(argv[4],"debug"))
		dq = 0;
	else if(!strcmp(argv[4],"quiet"))
		dq = 1;
	else {
		printf("USAGE: memsim <tracefile> <nframes> <vms|lru|clk|opt> <debug|quiet>\n");
		return 1;
	}

/* Read trace file */
	fp = fopen( tracefile, "r" );

       if(fp == NULL) {
    printf("Failed to open trace file! \n");
    return 1;
    }

    /* Set a pagetable */
    page_t *pagetable = pagetable_init(nframes);
    if(pagetable == NULL){
    	printf("Error: out of memory!!\n");
    	return 1;
    }
    /* for algorithm VMS, set clean list and dirty list */

    	page_t *clist = pagetable_init(3);
    	page_t *dlist = pagetable_init(3);
    	    if((clist == NULL) || (dlist == NULL)){
    	    	printf("Error: VMS list initiation failed!!\n");
    	    	return 1;
    	      }
 

    /* Debug */
    if(dq == 0){
    	printf("---------------Initialized Pagetable--------------\n");
    	pagetable_print(pagetable,nframes);
    	printf("--------------------------------------------------\n");
 
    }

    addr vaddr;
    char rw;
    int i;
    int pos = -2;	// the position of page found in table



    while ( fscanf( fp, "%x %c", &vaddr, &rw ) == 2 ){

    	/* Copy of file pointer for tracefile reading for OPT algorithm */

    		fp2 = fp;
    		fp_position = ftell(fp);

    	event_count++;

    	/* virtual address to virtual page number */
    	addr vpn = vaddr >> 12;
    	 if(dq == 0)
    		 fprintf(stderr, "read line %lld, page: %x, action: %c !\n",
        				event_count, vpn, rw);

    	/* To find the page in table */
    	pos = page_find(vpn,pagetable);

    	/*******************/
    	/*page not in table*/
    	/*******************/
    	if(pos == -1){

    		/*table is not full, insert in the end*/
    		if( table_length < nframes ){

    			page_insert(vpn, rw, pagetable, table_length);

    			read_count++;

    	    	  /* Debug */
    	    	if(dq == 0)
    	    	pagetable_print(pagetable,nframes);

                 /* Update table length after inserting new page */
    			 table_length++;

    		}

    		/*table is full, need to replace a page*/
    		else{
    			 /* Debug */
    			if(dq == 0){
    				printf("Table is full,  need to replace a page.\n");
    				if(algo == 2)
    					printf("CLOCK pointer: %d\n", pt);
    			}

    			/* Find position to replace page */
    			int position = -1;
    			if(algo == 0){

    			 vms(pagetable, clist, dlist, vpn, rw);

    			}
    			else{
    			if(algo == 1)position = lru(pagetable);
    			if(algo == 2)position = clk(pagetable);
    			if(algo == 3)position = opt(pagetable);


    			/* replace the page at position */
    			page_insert(vpn, rw, pagetable, position);

    			read_count++;
    			}


    			if(dq == 0){
    				pagetable_print(pagetable,nframes);
    				if(algo == 0){
    					printf("--------------------------------------------------\n");
    					pagetable_print(clist,3);
    					printf("--------------------------------------------------\n");
    					pagetable_print(dlist,3);
    					printf("--------------------------------------------------\n");
    				}

    			}

    		}
    	}
    	/*******************/
    	/***page in table***/
    	/*******************/
    	else if(pos >= 0){

    		/* Debug */
    		if(dq == 0)
     			printf("FIND this page in table position %d.\n ", pos);

     		pagetable[pos].usedtime = event_count;
     		pagetable[pos].clock = 1;
     		if(dq == 0)
     			pagetable_print(pagetable,nframes);
     
     		}

    	/* restore fp pointer to original position */
        	fseek ( fp , fp_position , SEEK_SET );


    }

    if(dq == 0){
    	printf("------------------finished---------------------\n");
    	pagetable_print(pagetable,nframes);
    }
    stat_print(nframes, event_count, read_count, write_count);

    fclose(fp);

    /* free pagetables */
    free(pagetable);
    free(clist);
    free(dlist);


    return 0;
}

/*******End of Main********************************************************************/
/*******End of Main********************************************************************/
/*******End of Main********************************************************************/



/* Initiate a pagetable */
page_t *pagetable_init(int nframe){
	page_t *newtable = (page_t*)malloc( nframes*sizeof (page_t) );
	int i;
	for (i = 0; i < nframes; i++){
		newtable[i].vpn = -1;
		newtable[i].rw = 'X';
		newtable[i].dirty = 'X';
		newtable[i].usedtime = 0;
		newtable[i].clock = 0;
		newtable[i].opt = 0;

	}
	return newtable;
}


/* Print a paget */
void page_print(page_t page){
	fprintf(stderr,
			"page: %x, action: %c, dirty: %c, used: %lld, clock: %d, opt: %lld\n",
			page.vpn, page.rw, page.dirty, page.usedtime, page.clock, page.opt);

}
/* Print a pagetable */
void pagetable_print(page_t *pagetable, int length){
	int i;
	for(i = 0; i < length; i++){
		fprintf(stderr, "Table position: %d  ", i);
		page_print(pagetable[i]);

	}
}

/* Find if a page is in pagetable */
int page_find(addr vpn, page_t *pagetable) {
	int i;
	for(i = 0; i < nframes; i++){
		if(vpn - pagetable[i].vpn == 0) return i;
	}
	return -1;
}
/* get a page from pagetable */
page_t *page_get(page_t *pagetable, int position){
	page_t *temp = pagetable_init(1);
	temp->clock = pagetable[position].clock;
	temp->dirty = pagetable[position].dirty;
	temp->opt = pagetable[position].opt;
	temp->rw = pagetable[position].rw;
	temp->usedtime =  pagetable[position].usedtime;
	temp->vpn =  pagetable[position].vpn;
	return temp;
}

/* Insert a page to pagetable */
int page_insert(addr vpn, char rw, page_t *pagetable, int position) {
	pagetable[position].vpn = vpn;
	pagetable[position].rw = rw;

	if(pagetable[position].dirty == 'd')
		write_count++;

	if(rw == 'W')
		pagetable[position].dirty = 'd';

	pagetable[position].usedtime = event_count;
	pagetable[position].clock = 1;


	return 0;
}

/* Print the final statistics to screen */
int stat_print(int nframes, unsigned long long event_count ,
		unsigned long long read_count,
		unsigned long long write_count ){
	fprintf(stderr,
			"total memory frames: %d \nevents in trace: %lld \ntotal disk reads: %lld \ntotal disk writes: %lld \n",
			nframes, event_count, read_count, write_count);
}

/********************************************************/
/*page repalcement algorithms vms(), lru(), clk(), opt()*/
/********************************************************/
int vms(page_t *pagetable, page_t *clist, page_t *dlist, addr vpn, char rw){
	int position = -1, list_pos = -1;

	if(rw == 'W'){
		/*find page in dlist copy it back to pagetable, no disk read, we will rewine
		 * disk read count
		 * */
 
			if (page_find(vpn,dlist) >= 0){
				 
				//position = fifo(pagetable, nframes, pt);
                                position = pt;
                                if (pt < nframes - 1)
                            		pt++;
                            	 else if (pt == nframes - 1)
                            	 	pt = 0;
            			    	if(pagetable[position].dirty == 'd')write_count--;
            			    	/* no write back happen when kick out a page
            			         * when kick out a page from table to list*/
				 
				page_insert(vpn, rw, pagetable, position);
			 
		   
			}

		    else{
		    /* page not found in dirty list
			 * put in table
			 * and kick out first-in page
			 * to dirty or clean list depends on it's dirty bit;
			 */
		    	 
		     

		    	//position = fifo(pagetable, nframes, pt);
		    		    	position = pt;
		    		    	 if (pt < nframes - 1)
		    		    			pt++;
		    		    		 else if (pt == nframes - 1)
		    		    		 	pt = 0;
		    

		    	page_t *temp = page_get(pagetable, position); /* malloc a temp page */
		    	//pagetable_print(temp,1);
		    	
		    	if(pagetable[position].dirty == 'd')write_count--;
		    	/* no write back happen when kick out a page
		         * when kick out a page from table to list*/
		    	page_insert(vpn, rw, pagetable, position);
		    	read_count++;
		 

		    	if(temp->dirty == 'd'){
		    		//list_pos = fifo(dlist,3,dirty_pt);
		    		list_pos = dirty_pt;
		    		if (dirty_pt < 2)dirty_pt++;
		    		 else if (dirty_pt == 2)dirty_pt = 0;		    				    		                            	 	
		    		                            	
		    		 
		    		page_insert(temp->vpn, temp->rw, dlist, list_pos);

		    	}
		    	if(temp->dirty == 'X'){
		    		//list_pos = fifo(clist,3,clean_pt);
		    		list_pos = clean_pt;
		    		if (clean_pt < 2)clean_pt++;
		    		else if (clean_pt == 2)clean_pt = 0;
		    				    		
		    	 
		    		page_insert(temp->vpn, temp->rw, clist, list_pos);
		    	}

		    	
		
		    	free(temp);


		    }

		}
	
	if(rw == 'R'){
			/*find page in dlist copy it back to pagetable, no disk read, we will rewine
			 * disk read count
			 * */
		 
				if (page_find(vpn,clist) >= 0){
			 
					//position = fifo(pagetable, nframes, pt);
	                                position = pt;
	                                if (pt < nframes - 1)
	                            		pt++;
	                            	 else if (pt == nframes - 1)
	                            	 	pt = 0;
	                
	                                if(pagetable[position].dirty == 'd')write_count--;
	            			    	/* no write back happen when kick out a page
	            			         * when kick out a page from table to list*/
	            			    	
					page_insert(vpn, rw, pagetable, position);
			    
					    
				}

			    else{
			    /* page not found in clean list
				 * put in table
				 * and kick out first-in page
				 * to dirty or clean list depends on it's clean bit;
				 */
			     
			    	 
		 
			    		    	 

			    	//position = fifo(pagetable, nframes, pt);
			    		    	position = pt;
			    		    	 if (pt < nframes - 1)
			    		    			pt++;
			    		    		 else if (pt == nframes - 1)
			    		    		 	pt = 0;
		 

			    	page_t *temp = page_get(pagetable, position); /* malloc a temp page */
			    	 
			    	
			    	if(pagetable[position].dirty == 'd')write_count--;
			    	/* no write back happen when kick out a page
			         * when kick out a page from table to list*/
		 
			    	page_insert(vpn, rw, pagetable, position);
			    	read_count++;
			      
			    				    

			    	if(temp->dirty == 'd'){
			    		//list_pos = fifo(dlist,3,clean_pt);
			    		list_pos = dirty_pt;
			    		if (dirty_pt < 2)dirty_pt++;
			    		 else if (dirty_pt == 2)dirty_pt = 0;		    				    		                            	 	
			    		                            	
			     
			    		page_insert(temp->vpn, temp->rw, dlist, list_pos);

			    	}
			    	if(temp->dirty == 'X'){
			    		//list_pos = fifo(clist,3,clean_pt);
			    		list_pos = clean_pt;
			    		if (clean_pt < 2)clean_pt++;
			    		else if (clean_pt == 2)clean_pt = 0;
			    				    		
			     
			    		page_insert(temp->vpn, temp->rw, clist, list_pos);
			    	}

			    	
			
			    	free(temp);


			    }

			}
 
	return 0;


}
int fifo(page_t *pagetable, int nframes, int pt){
	int position;
	/* the pt is the position of page to be replaced */
	position = pt;
	/* move pointer pt to the next */
	if (pt < nframes - 1)
		pt++;
	else if (pt == nframes - 1)
		pt = 0;
	return position;
}

/********************************************************/
int lru(page_t *pagetable){
	int i;
	unsigned long long lrutime = pagetable[0].usedtime;
	int position = 0;
	for(i = 1; i < nframes; i++){
		if( lrutime > pagetable[i].usedtime){
    	lrutime = pagetable[i].usedtime;
    	position = i;
		}
	}
	return position;
}

/********************************************************/
int clk(page_t *pagetable){

	int pointer;

	while (1) {

		if (pagetable[hand].clock == 0){
			pointer = hand;
			if (hand < nframes - 1)
							hand++;
					else if (hand == nframes - 1)
							hand = 0;
			return pointer;
		}

		else if (pagetable[hand].clock == 1){
			pagetable[hand].clock = 0;

					if (hand < nframes - 1)
						hand++;
					else if (hand == nframes - 1)
						hand = 0;
		}
		}
		return 1;/* should never get here */
}

/********************************************************/
int opt(page_t *pagetable){

	addr vaddr;
	char rw;
	int i, count = 0;
	int pos2, position = -1;
	unsigned long long ref = 0, refmax = 0;

	 while ( fscanf( fp2, "%x %c", &vaddr, &rw ) == 2 ) {

		 /* reset opt ref time in table */
		 for(i = 0; i < nframes; i++)pagetable[i].opt =  0;


	    	ref++;
	    	addr vpn = vaddr >> 12;

	    	pos2 = page_find(vpn,pagetable);

	    	/* a page in table is found in sequence, update it's ref time */
	    	if((pos2 >= 0) && (pagetable[pos2].opt == 0)){

	    		pagetable[pos2].opt = ref;


	    		refmax = ref;
	    		position = pos2;
	    		count++;
	    		/* all pages in table are searched and found in sequence */
	    		if( count >= nframes ){
	    			 return position;
	    		}

	    	}

	 }

	 /* None of pages in table is found in sequence */
	 if(count == 0){
		 return 0;
	 }
	 /* some pages in table are not found in sequence */
	 else if(count < nframes){


	 	    		for(i = 0; i < nframes; i++){
	 	    			/* replace the first none-referred page */
	 	    			if(pagetable[i].opt == 0)position = i;
	 	    		}

	 	    	 return position;
	 	    	}
	 /* reset ref time */


		 if(position < 0) return 0;

}
