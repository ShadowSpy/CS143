/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include "BTreeIndex.h"
#include "BTreeNode.h"

using namespace std;

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
    rootPid = -1;
}

/*
 * Open the index file in read or write mode.
 * Under 'w' mode, the index file should be created if it does not exist.
 * @param indexname[IN] the name of the index file
 * @param mode[IN] 'r' for read, 'w' for write
 * @return error code. 0 if no error
 */
RC BTreeIndex::open(const string& indexname, char mode)
{
  if(pf.open(indexname, mode))
    return 1;

  char info[PageFile::PAGE_SIZE];
  // Set the rootpid and tree height vars
  if (pf.endPid() == 0)
  {
    // Newly created file. Initialize height and root
    rootPid = -1;
    treeHeight = 0;

    // Reserve the first page of the page file for var storage
    // No need to actually store variables.
    // This will be done when file is closed
    if (pf.write(0, info))
    {
       return 2;
    }
  }
  else
  {
    if (pf.read(0, info))
    {
      return 2;
    }
    rootPid = *((PageId *)info);
    treeHeight = *((int *)(info+sizeof(PageId)));
  }

  return 0;
}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close()
{
    // Store info variables
    char info[PageFile::PAGE_SIZE];
    *((PageId *)info) = rootPid;
    *((int *)(info+sizeof(PageId))) = treeHeight;
    pf.write(0,info);

    // Close page file
    return pf.close();
}

/*
 * Recursive function for insert(key,rid)
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @param pid[IN] the pid for the node we are currently searching
 * @param height[IN] the height of the tree of node
 * @param ofKey[OUT] the new key to insert if overflow. Negative if no overflow
 * @return error code. 0 if no error
RC BTreeIndex::insert_helper(int key, const RecordId& rid, PageId pid, int height, int& ofKey, PageId& ofPid)
{
  ofKey = -1;

  // Base case: at leaf node
  if (height == treeHeight)
  {
    BTLeafNode ln;
    ln.read(pid, pf);
    if (ln.insert(key, rid))
    {
      // Overflow. Create new leaf node and split.
      BTLeafNode newNode;
      if (ln.insertAndSplit(key, rid, newNode, ofKey))
        return 1;
      ofPid = pf.endPid();
      if (newNode.write(ofPid, pf))
        return 1;
    }
    if (ln.write(pid, pf))
      return 1;
  }
  // Recursive: At non-leaf node
  else
  {
    BTNonLeafNode nln;
    int eid;
    PageId child;

    nln.read(pid, pf);
    nln.locate(key, eid);
    nln.readEntry(eid, child);
    insert_helper(key, rid, child, height+1, ofKey, ofPid);
    if (ofKey > 0)
    {
      // Child node overflowed. Insert (key,pid) into this node.
      if (nln.insert(ofKey, ofPid))
      {
        // Non-leaf node overflow. Split node between siblings.
        int midKey;
        BTNonLeafNode sibling;

        if (nln.insertAndSplit(ofKey, ofPid, sibling, midKey))
          return 1;
        ofKey = midKey;
        ofPid = pf.endPid();
        if (sibling.write(ofPid, pf))
          return 1;
      }
      else
      {
        ofKey = -1;
      }
      nln.write(pid, pf);
    }
  }
  return 0;
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid)
{
  int ofKey;
  PageId ofPid;

  if (insert_helper(key, rid, rootPid, 1, ofKey, ofPid))
    return 1;

  // If overflow at top level, create new root node
  if (ofKey > 0)
  {
    BTNonLeafNode newRoot;
    newRoot.initializeRoot(rootPid, ofKey, ofPid);
    rootPid = pf.endPid();
    treeHeight++;
    newRoot.write(rootPid, pf);
  }
  return 0;
}

/*
 * Find the leaf-node index entry whose key value is larger than or 
 * equal to searchKey, and output the location of the entry in IndexCursor.
 * IndexCursor is a "pointer" to a B+tree leaf-node entry consisting of
 * the PageId of the node and the SlotID of the index entry.
 * Note that, for range queries, we need to scan the B+tree leaf nodes.
 * For example, if the query is "key > 1000", we should scan the leaf
 * nodes starting with the key value 1000. For this reason,
 * it is better to return the location of the leaf node entry 
 * for a given searchKey, instead of returning the RecordId
 * associated with the searchKey directly.
 * Once the location of the index entry is identified and returned 
 * from this function, you should call readForward() to retrieve the
 * actual (key, rid) pair from the index.
 * @param key[IN] the key to find.
 * @param cursor[OUT] the cursor pointing to the first index entry
 *                    with the key value.
 * @return error code. 0 if no error.
 */
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor)
{
  PageId pid = rootPid;
  int i = 0;
  while (i < treeHeight-1)
  {
    int eid;
    BTNonLeafNode nln;

    nln.read(pid, pf);
    nln.locate(searchKey, eid);
    nln.readEntry(eid, pid);
    i++;
  }

  BTLeafNode ln;
  ln.read(pid, pf);
 
  cursor.pid = pid;
  ln.locate(searchKey, cursor.eid);

  return 0;
}

/*
 * Read the (key, rid) pair at the location specified by the index cursor,
 * and move foward the cursor to the next entry.
 * @param cursor[IN/OUT] the cursor pointing to an leaf-node index entry in the b+tree
 * @param key[OUT] the key stored at the index cursor location.
 * @param rid[OUT] the RecordId stored at the index cursor location.
 * @return error code. 0 if no error
 */
RC BTreeIndex::readForward(IndexCursor& cursor, int& key, RecordId& rid)
{

  BTLeafNode ln;
  ln.read(cursor.pid, pf);
  ln.readEntry(cursor.eid, key, rid);

  // Increment cursor
  if (cursor.eid < ln.getKeyCount())
    cursor.eid++;
  else
  {
    cursor.pid = ln.getNextNodePtr();
    cursor.eid = 0;
  }
}
