#include <iostream>
#include "BTreeNode.h"
#include "PageFile.h"
using namespace std;

void printNodeContents(BTLeafNode& node)
{
  int key;
  RecordId rid;

  for(int eid = 0; eid < node.getKeyCount(); eid++)
  {
    node.readEntry(eid,key,rid);
    cout << "Key: " << key
         << " Rid: (" << rid.pid << "," << rid.sid << ")" << endl;
  }
}

int main ()
{
  PageFile pf;
  BTLeafNode node;
  RecordId rid;
  int key;
  int eid;

  if (pf.open("roman.idx",'w'))
  {
    cout << "Could not open page file" << endl;
    return 1;
  }

  rid.pid = 1;
  rid.sid = 1;

  for (int i = 0; i < 85; i++)
  {
    node.insert(1,rid);
  }

  node.setNextNodePtr(31);

  key = node.getNextNodePtr();

  cout << "Next Node Ptr: " << key << endl;

  printNodeContents(node); 
}
