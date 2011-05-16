#include "BTreeNode.h"

using namespace std;


/*
 * Represents an entry within a leaf node
 */
struct BTLeafNode::Entry {
  RecordId rid;
  int key;
};

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf)
{
  return pf.read(pid, buffer);
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf)
{
  return pf.write(pid,buffer);
}

int BTLeafNode::getMaxKeyCount()
{
  return (PageFile::PAGE_SIZE-sizeof(PageId))/(sizeof(Entry));
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{
  int count = 0;
  Entry* entry = (Entry *) buffer;
  while(count < getMaxKeyCount()) {
    if (entry->key == 0)
      break;
    count++;
    entry++;
  }
  return count;
}

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid)
{
  int insertId;

  if (getKeyCount() >= getMaxKeyCount())
    return 1;  //Node is full
  if (locate (key, insertId))
    return 2;

  Entry* insertEntry = (Entry *)buffer + insertId;
  Entry* curEntry = (Entry *)buffer + getKeyCount();
  // This loop shifts Entrys to the right so we can insert the new one
  while (curEntry != insertEntry) {
    Entry* nextEntry = curEntry - 1;
    *curEntry = *nextEntry;
    curEntry = nextEntry;
  }

  // Insert new tuple into correct space
  insertEntry->key = key;
  insertEntry->rid = rid;
  return 0;
}

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, 
                              BTLeafNode& sibling, int& siblingKey)
{
  int eid; // index of where extra Entry should go
  int keyCount = getKeyCount();
  int siblingId = (keyCount+1)/2;

  Entry swap; // Entry object which holds extra Entry
  swap.key = key;
  swap.rid = rid;

  if (locate(swap.key, eid))
    return 2;

  //If before split, keep swapping until we get to split
  while (eid < siblingId) {
    Entry* cur = (Entry *)buffer + eid;
    Entry tmp = *cur;
    *cur = swap;
    swap = tmp;
    eid++;
  }
 
  //Set siblingKey if extra Entry is siblingId
  if (eid == siblingId)
    siblingKey = swap.key;
  else
    siblingKey = ((Entry *)buffer+siblingId)->key;

  //Insert extra tuple into sibling
  sibling.insert(swap.key, swap.rid);

  //Insert tuples after the split
  eid = siblingId; //eid is now just a loop index
  while (eid < keyCount) {
    Entry* cur = (Entry *)buffer + eid;

    sibling.insert(cur->key, cur->rid);
    cur->key = 0;
    eid++;
  }
  return 0;
}

/*
 * Find the entry whose key value is larger than or equal to searchKey
 * and output the eid (entry number) whose key value >= searchKey.
 * Remember that all keys inside a B+tree node should be kept sorted.
 * @param searchKey[IN] the key to search for
 * @param eid[OUT] the entry number that contains a key larger than or equalty to searchKey
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::locate(int searchKey, int& eid)
{
  eid = 0;
  while (eid < getKeyCount()) {
    Entry* entry = (Entry *)buffer + eid;
    if (searchKey > entry->key)
      eid++;
    else
      break;
  }

  // Make sure we haven't reached end of buffer
  if (eid == getMaxKeyCount()) {
    eid = -1;
    return 1;
  }
  return 0;
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid)
{
  if (eid < 0 || eid >= getKeyCount())
    return 1;

  Entry* entry = (Entry *)buffer + eid;
  rid = entry->rid;
  key = entry->key;
  return 0;
}

/*
 * Return the pid of the next sibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{
  PageId* pid = (PageId *)(buffer+PageFile::PAGE_SIZE) - 1;
  return *pid;
}

/*
 * Set the pid of the next sibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{
  PageId* ptr = (PageId *)(buffer+PageFile::PAGE_SIZE) - 1;
  *ptr = pid;
  return 0;
}

/*
 * Represents a Non-leaf entry
 */
struct BTNonLeafNode::Entry
{
  int key;
  PageId pid;
};

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf)
{
  return pf.read(pid, buffer);
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)
{
  return pf.write(pid, buffer);
}

/*
 * Return the max number of keys possible
 */
int BTNonLeafNode::getMaxKeyCount()
{
  return (PageFile::PAGE_SIZE-sizeof(PageId))/(sizeof(Entry));
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount()
{
  int count = 0;
  Entry* entry = (Entry *) buffer;
  while(count < getMaxKeyCount()) {
    if (entry->key == 0)
      break;
    count++;
    entry++;
  }
  return count;
}

/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{
  int insertId;

  if (getKeyCount() >= getMaxKeyCount())
    return 1;  //Node is full
  if (locate (key, insertId))
    return 2;

  insertId++; // We want to insert in the slot after the located one

  Entry* insertEntry = (Entry *)buffer + insertId;
  Entry* curEntry = (Entry *)buffer + getKeyCount();
  // This loop shifts Entrys to the right so we can insert the new one
  while (curEntry != insertEntry) {
    Entry* nextEntry = curEntry - 1;
    *curEntry = *nextEntry;
    curEntry = nextEntry;
  }

  // Insert new tuple into correct space
  insertEntry->key = key;
  insertEntry->pid = pid;
  return 0;
}

/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{
  int eid; // index of where extra Entry should go
  int keyCount = getKeyCount();
  int midId = keyCount/2;

  Entry swap; // Entry object which holds extra Entry
  swap.key = key;
  swap.pid = pid;

  if (locate(swap.key, eid))
    return 2;
  eid++;

  //If before split, keep swapping until we get to split
  while (eid < midId) {
    Entry* cur = (Entry *)buffer + eid;
    Entry tmp = *cur;
    *cur = swap;
    swap = tmp;
    eid++;
  }
 
  //Set midId
  if (eid != midId)
  {
    Entry *cur = (Entry *)buffer+midId;
    Entry temp = *cur;
    midKey = cur->key;
    *cur = swap;
    swap = temp;
  }

  midKey = swap.key;

  eid = midId; //eid is now just a loop index
  
  // Initialize root node for sibling
  Entry* cur = (Entry *)buffer + eid;
  sibling.initializeRoot(swap.pid,cur->key,cur->pid);
  cur->key = 0;
  eid++;

  //Insert tuples after the split
  while (eid < keyCount) {
    cur = (Entry *)buffer + eid;

    sibling.insert(cur->key, cur->pid);
    cur->key = 0;
    eid++;
  }
  return 0;
}

/*
 * Given the searchKey, find the child-node pointer to follow and
 * references it in eid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param eid[OUT] the entry number with the pointer
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locate(int searchKey, int& eid)
{
  eid = getKeyCount()-1;
  while (eid >= 0) {
    Entry* entry = (Entry *)buffer + eid;
    if (searchKey < entry->key)
      eid--;
    else
      break;
  }

  return 0;
}

/*
 * Read the (key, pid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, pid) pair from
 * @param key[OUT] the key from the entry
 * @param pid[OUT] the PageId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::readEntry(int eid, PageId& pid)
{
  if (eid >= getKeyCount())
    return 1;

  // Return the pointer not associated with an Entry
  if (eid < 0) {
    PageId *ptr = (PageId *)(buffer+PageFile::PAGE_SIZE-sizeof(PageId));
    pid = *ptr;
  }
  else {
    Entry* entry = (Entry *)buffer + eid;
    pid = entry->pid;
  }
  return 0;
}

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{
  if (getKeyCount() != 0)
    return 1;

  //A root entry is represented as follows:
  //  One Entry node, consisting of a key and a pointer (pid2)
  //  One pointer, which is stored at the end of the buffer, but
  //  represents the pointer to nodes smaller than the key
  //
  Entry root;
  root.key = key;
  root.pid = pid2;

  *((Entry *) buffer) = root;

  PageId *ptr1 = (PageId *)(buffer+PageFile::PAGE_SIZE-sizeof(PageId));
  *ptr1 = pid1;
  return 0;
}
