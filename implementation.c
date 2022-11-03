/*  

    Copyright 2018-21 by

    University of Alaska Anchorage, College of Engineering.

    Copyright 2022 by

    University of Texas at El Paso, Department of Computer Science.

    All rights reserved.

    Contributors:  Sergio Velasco
                   Dimitri Lyon
		   Elton Villa                 and
		   Christoph Lauter

    See file memory.c on how to compile this code.

    Implement the functions __malloc_impl, __calloc_impl,
    __realloc_impl and __free_impl below. The functions must behave
    like malloc, calloc, realloc and free but your implementation must
    of course not be based on malloc, calloc, realloc and free.

    Use the mmap and munmap system calls to create private anonymous
    memory mappings and hence to get basic access to memory, as the
    kernel provides it. Implement your memory management functions
    based on that "raw" access to user space memory.

    As the mmap and munmap system calls are slow, you have to find a
    way to reduce the number of system calls, by "grouping" them into
    larger blocks of memory accesses. As a matter of course, this needs
    to be done sensibly, i.e. without wasting too much memory.

    You must not use any functions provided by the system besides mmap
    and munmap. If you need memset and memcpy, use the naive
    implementations below. If they are too slow for your purpose,
    rewrite them in order to improve them!

    Catch all errors that may occur for mmap and munmap. In these cases
    make malloc/calloc/realloc/free just fail. Do not print out any 
    debug messages as this might get you into an infinite recursion!

    Your __calloc_impl will probably just call your __malloc_impl, check
    if that allocation worked and then set the fresh allocated memory
    to all zeros. Be aware that calloc comes with two size_t arguments
    and that malloc has only one. The classical multiplication of the two
    size_t arguments of calloc is wrong! Read this to convince yourself:

    https://bugzilla.redhat.com/show_bug.cgi?id=853906

    In order to allow you to properly refuse to perform the calloc instead
    of allocating too little memory, the __try_size_t_multiply function is
    provided below for your convenience.
    
*/

#include <stddef.h>
#include <sys/mman.h>

/* Predefined helper functions */

static void *__memset(void *s, int c, size_t n) {
  unsigned char *p;
  size_t i;

  if (n == ((size_t) 0)) return s;
  for (i=(size_t) 0,p=(unsigned char *)s;
       i<=(n-((size_t) 1));
       i++,p++) {
    *p = (unsigned char) c;
  }
  return s;
}

static void *__memcpy(void *dest, const void *src, size_t n) {
  unsigned char *pd;
  const unsigned char *ps;
  size_t i;

  if (n == ((size_t) 0)) return dest;
  for (i=(size_t) 0,pd=(unsigned char *)dest,ps=(const unsigned char *)src;
       i<=(n-((size_t) 1));
       i++,pd++,ps++) {
    *pd = *ps;
  }
  return dest;
}

/* Tries to multiply the two size_t arguments a and b.

   If the product holds on a size_t variable, sets the 
   variable pointed to by c to that product and returns a 
   non-zero value.
   
   Otherwise, does not touch the variable pointed to by c and 
   returns zero.

   This implementation is kind of naive as it uses a division.
   If performance is an issue, try to speed it up by avoiding 
   the division while making sure that it still does the right 
   thing (which is hard to prove).

*/
static int __try_size_t_multiply(size_t *c, size_t a, size_t b) {
  size_t t, r, q;

  /* If any of the arguments a and b is zero, everthing works just fine. */
  if ((a == ((size_t) 0)) ||
      (b == ((size_t) 0))) {
    *c = a * b;
    return 1;
  }

  /* Here, neither a nor b is zero. 

     We perform the multiplication, which may overflow, i.e. present
     some modulo-behavior.

  */
  t = a * b;

  /* Perform Euclidian division on t by a:

     t = a * q + r

     As we are sure that a is non-zero, we are sure
     that we will not divide by zero.

  */
  q = t / a;
  r = t % a;

  /* If the rest r is non-zero, the multiplication overflowed. */
  if (r != ((size_t) 0)) return 0;

  /* Here the rest r is zero, so we are sure that t = a * q.

     If q is different from b, the multiplication overflowed.
     Otherwise we are sure that t = a * b.

  */
  if (q != b) return 0;
  *c = t;
  return 1;
}

/* End of predefined helper functions */

/* Your helper functions 

   You may also put some struct definitions, typedefs and global
   variables here. Typically, the instructor's solution starts with
   defining a certain struct, a typedef and a global variable holding
   the start of a linked list of currently free memory blocks. That 
   list probably needs to be kept ordered by ascending addresses.

*/
struct __memory_block_struct_t{
  size_t size;
  void *mmap_start;
  size_t mmap_size;
  struct __memory_block_struct_t *next;
};typedef struct __memory_block_struct_t memory_block_t;

#define __MEMORY_MAP_MIN_SIZE ((size_t) (16777216))

static memory_block_t *free_memory_blocks = NULL;

/*static void __get_memory_map(size_t rawsize){
  size_t size, nmemb;
  memory_block_t *curr,*new, *prev;

  if (rawsize == ((size_t) 0)) return NULL;

  size = rawsize - ((size_t) 1);
  nmemb = size + sizeof(mem_block_t);
  if(nmemb <size) return NULL;

  nmemb /= sizeof(memory_block_t); 
  if(!__try_size_t_multiply(&size, nmemb, sizeof(memory_block_t))) return NULL;

  for(curr = free_memory_blocks, prev = NULL;
      curr!= NULL;
      curr = (prev = curr)->next){
    if(curr->size >=size){
      if((curr->size -size) < sizeof(memory_block_t)){
	if(prev==NULL) free_mem_blocks = curr->next;
	else prev->next = curr->next;
	return curr;
      } else{
	new = (memory_block_t *) (((void *) curr)+size);
	new->size = curr->size - size;
	new->mmap_start = curr->mmap_start;
	new->mmap_size = curr->mmap_size;
	new->next = curr->next;

	if(prev==NULL) free_memory_blocks = new;
	else prev->next = new;

	curr->size = size;
	return NULL;
      }
    }
  }
  return NULL;
  }*/

static void __prune_memory_maps() {
  memory_block_t *curr, *prev, *next;
  for (curr = free_memory_blocks, prev = NULL;
       curr!=NULL; curr = next){
    next = curr->next;
    if ((curr->size == curr->mmap_size) &&
	(curr->mmap_start == ((void *) curr))) {
      
      if (munmap (curr->mmap_start, curr->mmap_size) == 0) {
	if (prev == NULL)
	  free_memory_blocks = next;
	else
	  prev->next = next;
      }
    }
    prev = curr;
  }
}

static void __coalesce_memory_blocks(memory_block_t *ptr, int prune){
  memory_block_t *clobbered;
  int did_coalesce;
  if(ptr==NULL || (ptr->next==NULL)){
    if(prune){
      __prune_memory_maps();
    }
    return;
  }
  did_coalesce = 0;

  if((ptr->mmap_start==ptr->next->mmap_start) &&
     ((((void *) ptr) + ptr->size)==((void *)(ptr->next)))){
    clobbered=ptr->next;
    ptr->next = clobbered->next;
    ptr->size = clobbered->size;
    did_coalesce=1;
  }
  if(ptr->next==NULL){
    if(did_coalesce && prune){
      __prune_memory_maps();
    }
    return;
  }
  if(ptr->next->next==NULL){
    if(did_coalesce && prune){
      __prune_memory_maps();
    }
    return;
  }
  if((ptr->next->mmap_start==ptr->next->next->mmap_start) &&
     ((((void *) ptr) +ptr->size) == ((void *) (ptr->next)))){
    clobbered = ptr->next->next;
    ptr->next->next = clobbered->next;
    ptr->next->size += clobbered->size;
    did_coalesce = 1;
  }
  if(did_coalesce && prune){
    __prune_memory_maps();
  }
  return;
}

static void __add_free_memory_block(memory_block_t *ptr, int prune){
  memory_block_t *curr, *prev;
  if(ptr==NULL) return;
  for(curr=free_memory_blocks, prev=NULL; curr!=NULL; curr=(prev=curr)->next){
    if(((void *)ptr)<((void *) curr)){
      break;
    }
  }
  if(prev==NULL){
    ptr->next = free_memory_blocks;
    free_memory_blocks = ptr;
    __coalesce_memory_blocks(ptr,prune);
  } else{
    ptr->next = curr;
    prev->next = ptr;
    __coalesce_memory_blocks(prev,prune);
  }
}

static void __new_memory_map(size_t rawsize){
  size_t size, minnmemb, nmemb;
  void *ptr;
  memory_block_t *new;

  if(rawsize == ((size_t) 0))  return;

  size = rawsize - ((size_t) 1);
  nmemb = size + sizeof(memory_block_t);
  if(nmemb < size) return;

  nmemb /= sizeof(memory_block_t);
  minnmemb = __MEMORY_MAP_MIN_SIZE / sizeof(memory_block_t);
  if(nmemb < minnmemb) nmemb = minnmemb;
  if(!__try_size_t_multiply(&size, nmemb, sizeof(memory_block_t))) return;

  ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

  if(ptr == MAP_FAILED) return;

  new = (memory_block_t*) ptr;
  new->size = size;
  new->mmap_start = ptr;
  new->mmap_size = size;
  new->next = NULL;

  __add_free_memory_block(new, 0);
}



static memory_block_t * __get_memory_block(size_t rawsize) {
  size_t nmemb, size;
  memory_block_t *curr, *new, *prev;
  
  if (rawsize == ((size_t) 0)) return NULL;
  
  size = rawsize-((size_t)1);
  nmemb = size + sizeof(memory_block_t);
  if (nmemb < size) return NULL;

  nmemb /= sizeof(memory_block_t);
  if (!__try_size_t_multiply(&size, nmemb, sizeof(memory_block_t))) return NULL;

  for(curr = free_memory_blocks, prev = NULL;
      curr != NULL;
      curr = (memory_block_t *)(prev = curr)->next) {
    if (curr->size >= size) {
      if ((curr->size - size) < sizeof(memory_block_t)) {
	if (prev == NULL) {
	  free_memory_blocks = curr->next;
	}else{
	  prev->next = curr->next;
	}
	return curr;
      }else{
	new = (memory_block_t *) (((void *) curr) + size);
	new->size = curr->size - size;
	new->mmap_start = curr->mmap_start;
	new->mmap_size = curr->mmap_size;
	new->next = curr->next;
	if (prev == NULL) {
	  free_memory_blocks = new;
	}else{
	  prev->next = new;
	}
	curr->size = size;
	return curr;
      }
    }
  }
  return NULL;
}


/* End of your helper functions */

/* Start of the actual malloc/calloc/realloc/free functions */

void __free_impl(void *);

void *__malloc_impl(size_t size) {
  size_t s;
  void *ptr;
  if(size==(size_t) 0) return NULL;
  s=size+sizeof(memory_block_t); 
  if(s<size) return NULL;
  ptr=(void *) __get_memory_block(s);
  if(ptr!=NULL){
    return ptr+sizeof(memory_block_t);
  }
  __new_memory_map(s);
  ptr=(void *) __get_memory_block(s);
  if(ptr!=NULL){
    return ptr+sizeof(memory_block_t);
  }
  return NULL;
}

void *__calloc_impl(size_t nmemb, size_t size) {
  size_t s;
  void *ptr;
  if(!__try_size_t_multiply(&s,nmemb,size)) return NULL;
  ptr = __malloc_impl(s);
  if(ptr==NULL){
    __memset(ptr,0,s);
  }
  return ptr;  
}

void *__realloc_impl(void *ptr, size_t size) {
  void *new_ptr;
  memory_block_t *old_mem_block;
  size_t s;
  if(ptr==NULL){
    __malloc_impl(size);
  }
  if(size==(size_t) 0){
    __free_impl(ptr);
    return NULL;
  }
  new_ptr=__malloc_impl(size);
  if(new_ptr==NULL) return NULL;
  old_mem_block = (memory_block_t *) (ptr-sizeof(memory_block_t));
  s = old_mem_block->size;
  if(size<s) s = size;
  __memcpy(new_ptr, ptr,s);
  __free_impl(ptr);
  return new_ptr;
}

void __free_impl(void *ptr) {
  if(ptr==NULL) return;
  __add_free_memory_block(ptr-sizeof(memory_block_t),1);
}

/* End of the actual malloc/calloc/realloc/free functions */

