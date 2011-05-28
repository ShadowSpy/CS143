#include <iostream>
#include "BTreeNode.h"
#include "PageFile.h"
using namespace std;

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

  //rid.pid = 12;
  //rid.sid = 34;

  node.read(0, pf);

  cout << "# Keys: " << node.getKeyCount() << " Max Keys: " << node.getMaxKeyCount() << endl;

  if (node.locate(88, eid))
  {
    cout << "Could not locate" << endl;
    return 0;
  }
  node.readEntry(eid, key, rid);

  cout << "Key: " << key << " Pid: " << rid.pid << " Sid: " << rid.sid << endl;
}
