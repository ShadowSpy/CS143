#include <iostream>
#include "BTreeIndex.h"
#include "RecordFile.h"
using namespace std;

int main()
{
  BTreeIndex index;
  IndexCursor cursor;
  RecordFile rf;

  int key;
  string value;
  RecordId rid;
  int count;

  if(rf.open("xsmall.tbl",'r'))
    cout << "Could not open record file" << endl;

  if(index.open("xsmall.idx", 'w'))
  {
    cout << "Index failed to open" << endl;
  }

  if(index.locate(0,cursor))
    cout << "Could not locate cursor" << endl;

  count = 0;

  while (!index.readForward(cursor, key, rid))
  {
    cout << "Rid: (" << rid.pid << "," << rid.sid << ") ";
    rf.read(rid, key, value);
    cout << "Key: " << key << " Value: " << value << endl;
    count++;
  }

  cout << count << endl;
}
