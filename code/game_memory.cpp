
//
// Game Memory
//

#define M_ARENA_DEFAULT_ALIGNMENT (2*sizeof(void*))

//
// NOTE: Memory arena.
//
struct M_Arena {
  u8* base;
  s64 size;
  s64 pos, prev_pos;
};

struct M_Arena_Frame {
  M_Arena* arena;
  s64 pos;
};

M_Arena m_arena(u8* base, s64 size) {
  M_Arena r = {};
  r.base = base;
  r.size = size;
  return r;
}

s64 do_memory_alignment(s64 size, s64 alignment) {
  s64 r = alignment*(size/alignment) + alignment;
  return r;
}

u8 *m_arena_alloc(M_Arena *arena, s64 size, s64 alignment = M_ARENA_DEFAULT_ALIGNMENT) {
  u8 *r = NULL;

  s64 size_aligned = do_memory_alignment(size, alignment);
  if(arena->pos + size_aligned > arena->size)
    Assert(!"Can't fit alloc size!!");
  
  r = arena->base + arena->pos;
  arena->prev_pos = arena->pos;
  arena->pos += size_aligned;
  
  zero_memory(r, size_aligned);
  
  return r;
}

#define m_arena_struct(arena, type)       (type *)m_arena_alloc(arena, sizeof(type))
#define m_arena_array(arena, type, count) (type *)m_arena_alloc(arena, sizeof(type)*count)

void m_arena_clear(M_Arena *arena) {
  arena->pos = 0;
  arena->prev_pos = 0;
}

M_Arena_Frame m_arena_start_frame(M_Arena *arena) {
  M_Arena_Frame r = {arena, arena->pos};
  return r;
}

void m_arena_end_frame(M_Arena *arena, M_Arena_Frame frame) {
  Assert(frame.arena == arena);
  arena->pos = frame.pos;
}



//
// NOTE: Memory free list allocator.
//

//Allocation_Header and Free_List_Entry get intruded by each other.
struct Allocation_Header {
  u64 pos, size;
  Allocation_Header* next;
};

struct Free_List_Entry {
  u64 pos, size;
  Free_List_Entry* next;
};

struct Free_List {
  Free_List_Entry* first;
};

struct Allocation_List {
  Allocation_Header* first;
};

struct Allocator {
  u8* base;
  u64 size;

  Allocation_List alloc_list;
  Free_List free_list;
};

Allocator allocator_create(u8* base, u64 size) {
  Allocator allocator = {};

  if(size <= sizeof(Allocation_Header)) Assert(!"Allocator is WAY to small!!!!");
  
  allocator.base = base;
  allocator.size = size;

  allocator.free_list.first = (Free_List_Entry*)allocator.base;
  
  Free_List_Entry* first = allocator.free_list.first;
  first->pos = sizeof(Free_List_Entry);
  first->size = size - sizeof(Free_List_Entry);
  first->next = NULL;
  
  return allocator;
}

u8* allocator_alloc(Allocator* allocator, u64 desired_size) {
  
  Free_List_Entry* best = NULL;
  Free_List_Entry* prev = NULL;
  
  Free_List_Entry* curr = allocator->free_list.first;
  Free_List_Entry* curr_prev = NULL;

  // Finding the best fittting entry.
  for(; curr ; curr_prev = curr, curr = curr->next) {
    if(curr->size < desired_size) continue;
    if(!best) {
      best = curr;
      prev = curr_prev;
      continue;
    }

    u64 curr_fit = curr->size - desired_size;
    u64 best_fit = best->size - desired_size;
    
    if(curr_fit < best_fit) {
      best = curr;
      prev = curr_prev;
    }
  }
  
  if(!best) return NULL;

  u64 alloc_pos  = best->pos;
  u64 alloc_size = best->size;

  // Creating an new free list entry if there is space left remaining from the best block.
  // If there isn't space the the entry is just the "next of best".
  Free_List_Entry* entry = best->next;
  
  u64 size_left  = best->size - desired_size;
  if(size_left > sizeof(Free_List_Entry)) {
    u8* entry_address = allocator->base + best->pos + desired_size;
    entry = (Free_List_Entry*)entry_address;
    
    entry->pos = best->pos + desired_size + sizeof(Free_List_Entry);
    entry->size = size_left - sizeof(Free_List_Entry);

    // We have a new entry, so we need to retain the "next of best".
    entry->next = best->next;

    alloc_size = desired_size;
  }

  if(prev) prev->next = entry;
  else     allocator->free_list.first = entry;
  
  // Init new allocation header and add to alloc_list.
  Allocation_Header* header = (Allocation_Header*)best;
  header->pos  = alloc_pos;
  header->size = alloc_size;
  header->next = NULL;

  // Inserting in a sorted manner, we want the header list to be sorted by position.
  Allocation_Header* curr_alloc = allocator->alloc_list.first;
  Allocation_Header* prev_alloc = NULL;
  for(; curr_alloc ;prev_alloc = curr_alloc, curr_alloc = curr_alloc->next) {
    if(header->pos < curr_alloc->pos) break;
  }

  header->next = curr_alloc;
  if(prev_alloc) prev_alloc->next = header;
  else           allocator->alloc_list.first = header;
  
  u8* result = allocator->base + header->pos;
  zero_memory(result, alloc_size);

  return result;
}

#define allocator_alloc_struct(allocator, type)       (type*)allocator_alloc(allocator, sizeof(type))
#define allocator_alloc_array(allocator, type, count) (type*)allocator_alloc(allocator, sizeof(type)*count)

void allocator_free(Allocator* allocator, void* ptr) {
  if(ptr == NULL) return;
  
  u8* ptr8 = (u8* )ptr;
  
  u8* header_address = ptr8 - sizeof(Allocation_Header);
  Allocation_Header* header = (Allocation_Header*)header_address;
  
  // Remove allocation header from list.
  Allocation_Header* curr_alloc = allocator->alloc_list.first;
  Allocation_Header* prev_alloc = NULL;
  for(; curr_alloc ; prev_alloc = curr_alloc, curr_alloc = curr_alloc->next) {
    if(curr_alloc->pos == header->pos) break;
  }
  
  if(prev_alloc) prev_alloc->next = curr_alloc->next;
  else           allocator->alloc_list.first = curr_alloc->next;
  
  // Insert sorted by position and merge if can.
  Free_List_Entry* entry = (Free_List_Entry*)header;
  
  Free_List_Entry* curr = allocator->free_list.first;
  Free_List_Entry* prev = NULL;
  for(; curr ; prev = curr, curr = curr->next) {
    if(entry->pos < curr->pos) break;
  }

  if(curr) { 
    // Merging with curr block
    b32 is_consecutive = (entry->pos + entry->size + sizeof(Free_List_Entry) == curr->pos);
    if(is_consecutive) {
      entry->size += sizeof(Free_List_Entry) + curr->size;
      entry->next = curr->next;
    }else { 
      entry->next = curr;
    }
  } else {
    entry->next = NULL;
  }
  
  if(prev) {
    // Merging with prev block
    b32 is_consecutive = (prev->pos + prev->size + sizeof(Free_List_Entry) == entry->pos);
    if(is_consecutive) {
      prev->size += entry->size + sizeof(Free_List_Entry);
      prev->next = entry->next;
    } else {
      prev->next = entry;
    }
  }else {
    allocator->free_list.first = entry;
  }
}
