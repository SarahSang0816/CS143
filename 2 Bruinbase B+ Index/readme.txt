In this project, we develop a B+ tree to store key information of data. The structure is as follows.
non-leaf node structure:
|# keys(4 byte)|, |PageId(4 byte) key|, |PageId(4 byte) key|...., |PageId(4 byte) key|, |PageId|
 leaf node structure:
|# keys(4 byte)|, |PageId(4 byte)|, |key(4 byte) RecordId(pid, sid)|, |key(4 byte) RecordId(pid, sid)|....
 For each node, its content is stored in page file.

Algorithm:
  1. check leaf node is full or not
  2. if not full, find the location to insert the new pair
  3. copy the right part of the node at the insertion position to a temp
  4. insert new (record, key) pair at the location
  5. copy back the temp part after the new pair
  6. ++keyCount
 * Note: leaf node structure
 |Number of keys(4 byte)|, |PageId(4 byte)|, |key(4 byte) RecordId(pid, sid)|, |key(4 byte) RecordId(pid, sid)|....

Insert and split: 
consider different situations of even and odd number of keys

In final part, we add index to load function of sqlEngine class and modify select function to use B+ tree. First we need to determine whether to use the tree index or just direct scan. Under circumstances that ¡°select key¡± or ¡°select count(*)¡±, or where conditions involve key with >, <, >=, = and <=, B+ tree should be used. Otherwise, it should just go to direct scan. For example, if there¡¯s only condition with non-equality on keys or on values, direct scan will be executed. Since the root pid is at index = 1, the pages read are a little bit more than that of output file. But the difference is minor. 

Yu Zhang yuzhang93@gmail.com
Qi Sang sarahsang0816@gmail.com