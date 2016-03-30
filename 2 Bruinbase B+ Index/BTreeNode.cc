/*
 non-leaf node structure:
 |# keys(4 byte)|, |PageId(4 byte) key|, |PageId(4 byte) key|...., |PageId(4 byte) key|, |PageId|
 leaf node structure:
 |# keys(4 byte)|, |PageId(4 byte)|, |key(4 byte) RecordId(pid, sid)|, |key(4 byte) RecordId(pid, sid)|....
 For each node, its content is stored in page file,
 */
#include "BTreeNode.h"

/* constructor, set all values in buffer to 0 */
BTLeafNode::BTLeafNode()
{
    memset(buffer, 0, PageFile::PAGE_SIZE);
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */

RC BTLeafNode::read(PageId pid, const PageFile& pf)
{
    RC rc;
    if (pid == 0) {
        rc = RC_INVALID_PID;
        return rc;
    }
    memset(buffer, 0, PageFile::PAGE_SIZE);
    if ((rc = pf.read(pid, buffer)) < 0) {
        return RC_FILE_READ_FAILED ;
    }
    return 0;
    
}

/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf)
{
    return pf.write(pid, buffer);
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{
    int count = 0;
    //first four bytes stores # of keys(or records) in a page
    memcpy(&count, &buffer, sizeof(int));
    return count;
}

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 --------------------------------------------------------------------
 * Algorithm:
 1. check leaf node is full or not
 2. if not full, find the location to insert the new pair
 3. copy the right part of the node at the insertion position to a temp
 4. insert new (record, key) pair at the location
 5. copy back the temp part after the new pair
 6. ++keyCount
 * Note: leaf node structure
 |Number of keys(4 byte)|, |PageId(4 byte)|, |key(4 byte) RecordId(pid, sid)|, |key(4 byte) RecordId(pid, sid)|....
 */
RC BTLeafNode::insert(int key, const RecordId& rid)
{
    // 1. check leaf node is full or not
    int slot_size = sizeof(int) + sizeof(RecordId);
    int used_size = sizeof(int) + sizeof(PageId) + getKeyCount() * slot_size;
    int remaining_size = PageFile::PAGE_SIZE - used_size;
    
    if(remaining_size < slot_size) {
        return RC_NODE_FULL;
    }
    
    // 2. find the location to insert the new (record,key) pair
    int insertPosition = getKeyCount();
    RC rc = locate(key, insertPosition);
    if (rc == 0) {
        return 0;
    }
    
    // First skip the initial key count and pageId in the leaf node structure
    char* iter = &(buffer[0]);
    iter += sizeof(int);
    iter += sizeof(PageId);
    
    // 3. move to correct position insert the new (key, record) pair
    for (int i = 0; i < insertPosition; i++) {
        iter += slot_size;
    }
    
    // 4. copy the right part of the node at the insertion position to a temp
    // allocate a new page temp to store the right part of the node at the insertion position
    char* temp = (char*)malloc(PageFile::PAGE_SIZE * sizeof(char));
    // initialize the new page values to zero
    memset(temp, 0, PageFile::PAGE_SIZE);
    
    bool insert_at_end = (insertPosition == getKeyCount());
    // if the new pair is not inserted at the end, copy the right part to temp
    if (insert_at_end == false) {
        // copy the right part to temp
        memcpy(temp, iter, slot_size * (getKeyCount() - insertPosition));
        // set the original right part to zero
        memset(iter, 0, slot_size * (getKeyCount() - insertPosition));
    }
    
    memcpy(iter, &key, sizeof(int));
    iter += sizeof(int);
    memcpy(iter, &rid, sizeof(RecordId));
    iter += sizeof(RecordId);
    
    // 5. if the new pair is not inserted at the end, copy back the temp part back after the new pair
    if (insert_at_end == false) {
        memcpy(iter, temp, slot_size * (getKeyCount() - insertPosition));
    }
    free(temp);
    
    // 6. increase the keyCount by 1
    // move the iter, point to the # key
    iter = &(buffer[0]);
    // increase the keyCount by 1
    int currKeyCount = getKeyCount();
    currKeyCount++;
    // store the new keyCount value to # key (the first 4 byte)
    memcpy(iter, &currKeyCount, sizeof(int));
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
    int nkeys = getKeyCount();
    int slot_size = sizeof(int) + sizeof(RecordId);
    
    char* iter = &(buffer[0]);
    iter += sizeof(int); // # of keys
    iter += sizeof(PageId);
    
    int i = 0; // ith key
    while(i < nkeys / 2){
        iter += slot_size;
        i++;
    }
    int midkey;
    memcpy(&midkey, iter, sizeof(int)); // obtain the midkey
    char *tmp = (char*)malloc(1024 * sizeof(char)); // allocate a temp space
    memset(tmp, 0, 1024 * sizeof(char));
    int nkeysIntmp = 0;
    
    if(key > midkey){
        // copy the second half past the midkey, to guarantee left has one more key than right
        iter += slot_size;
        if(nkeys % 2 == 0){ // even number of keys
            nkeysIntmp = nkeys/2-1;
            memcpy(tmp, iter, slot_size * (nkeys/2-1));
            memset(iter, 0, slot_size * (nkeys/2-1));
            
        }
        else{ // odd number of keys
            nkeysIntmp = nkeys/2;
            memcpy(tmp, iter, slot_size * (nkeys/2));
            memset(iter, 0, slot_size * (nkeys/2));
        }
        
        // update number of keys
        *(int *)buffer = nkeys/2 + 1;
        
        if(sibling.insert(key, rid) != 0) { // insert into sibling
            return RC_FILE_WRITE_FAILED;
        }
    }
    else{
        if(nkeys % 2 == 0){ // even number of keys
            nkeysIntmp = nkeys/2;
            memcpy(tmp, iter, slot_size * (nkeys/2));
            memset(iter, 0, slot_size * (nkeys/2));
            
        }
        else{ // odd number of keys
            nkeysIntmp = nkeys/2+1;
            memcpy(tmp, iter, slot_size * (nkeys/2+1));
            memset(iter, 0, slot_size * (nkeys/2+1));
        }
        
        // update number of keys
        *(int *)buffer = nkeys/2;
        
        if(insert(key, rid) != 0) { // insert into the original node
            return RC_FILE_WRITE_FAILED;
        }
    }
    
    // copy tmp to sibling node
    char* tempIter = tmp;
    for(int i = 0; i < nkeysIntmp; i++){
        int key;
        RecordId r;
        memcpy(&key, tempIter, sizeof(int)); // retrieve key

        tempIter += sizeof(int);
        memcpy(&r, tempIter, sizeof(RecordId));
        tempIter += sizeof(RecordId);
        
        if(sibling.insert(key, r) != 0) {
            return RC_FILE_WRITE_FAILED;
        }
    }
    free(tmp);
    
    // store the siblingkey
    RecordId rec;
    sibling.readEntry(0, siblingKey, rec);
    return 0;
}

/**
 * If searchKey exists in the node, set eid to the index entry
 * with searchKey and return 0. If not, set eid to the index entry
 * immediately after the largest index key that is smaller than searchKey,
 * and return the error code RC_NO_SUCH_RECORD.
 * Remember that keys inside a B+tree node are always kept sorted.
 * @param searchKey[IN] the key to search for.
 * @param eid[OUT] the index entry number with searchKey or immediately
 behind the largest key smaller than searchKey.
 * @return 0 if searchKey is found. If not, RC_NO_SEARCH_RECORD.
 */
RC BTLeafNode::locate(int searchKey, int& eid)
{
    // First skip the initial key count and pageId in the leaf node structure
    char* iter = &(buffer[0]);
    iter += sizeof(int);
    iter += sizeof(PageId);
    int slot_size = sizeof(RecordId) + sizeof(int);
    int currKey = 0;
    eid = getKeyCount();
    for (int i = 0; i < getKeyCount(); i++) {
        // Read current key
        memcpy(&currKey, iter, sizeof(int));
        
        // found the insertion location (###########Could the input key be 0?)
        if (currKey == searchKey) {
            eid = i;
            return 0;
        } else if (currKey > searchKey) {
            eid = i;
            break;
        }
        // move to the next pair
        iter += slot_size;
    }
    return RC_NO_SUCH_RECORD;
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
    // First skip the initial key count and pageId in the leaf node structure
    int slot_size = sizeof(RecordId) + sizeof(int);
    char* iter = &(buffer[0]);
    iter += sizeof(int);
    iter += sizeof(PageId);
    
    // Skip previous keys
    for (int i = 0; i < eid; i++) {
        iter += slot_size;
    }
    
    // Read entry values
    memcpy(&key, iter, sizeof(int));
    iter += sizeof(int);
    memcpy(&rid, iter, sizeof(RecordId));
    return 0;
}

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node
 */
PageId BTLeafNode::getNextNodePtr()
{
    //get the next pid from the PageFile, will be null if no next node
    PageId pid;
    
    char* iter = &(buffer[0]);
    iter += sizeof(int);
    
    memcpy(&pid, iter, sizeof(PageId));
    
    return pid;
}

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{
    //on split, call this function to set pointer to the next node at the end of the buffer
    char* iter = &(buffer[0]);
    iter += sizeof(int);
    
    memcpy(iter, &pid, sizeof(PageId));
    
    return 0;
}








/*******************BTNonLeafNode*********************/

BTNonLeafNode::BTNonLeafNode() {
    memset(buffer, 0, PageFile::PAGE_SIZE);
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf)
{
    //store PageFile and PageId that this node corresponds to, makes life easier
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
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount()
{
    int count = 0;
    //first four bytes stores number of keys in a non-leaf node
    memcpy(&count, &buffer, sizeof(int));
    
    return count;
}


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 * Note: non-leaf node structure
 |Number of keys(4 byte)|, |PageId|, |key, PageId(4 byte)|, |key, PageId(4 byte)|, |key, PageId(4 byte)|....
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{
    // 1. check non-leaf node is full or not
    int slot_size = sizeof(int) + sizeof(PageId);
    int used_size = sizeof(int) + sizeof(PageId) + getKeyCount() * slot_size;
    int remaining_size = PageFile::PAGE_SIZE - used_size;
    
    if(remaining_size < slot_size) {
        return RC_NODE_FULL;
    }
    
    // 2. find the location to insert the new (record,key) pair
    // move the iter to the first key in the node
    char* iter = &(buffer[0]);
    iter += sizeof(int);
    iter += sizeof(PageId);
    int position = getKeyCount();
    int currKey = 0;
    for (int i = 0; i < getKeyCount(); i++) {
        // Read current key
        memcpy(&currKey, iter, sizeof(int));
        // found the insertion location (###########Could the input key be 0?)
        if (currKey == key) {
            return 0;
        } else if (currKey > key) {
            position = i;
            break;
        }
        // move to the next pair
        iter += slot_size;
    }
    
    // 3. move to the correct position, and insert the new (key, PageId) pair
    iter = &(buffer[0]);
    for (int i = 0; i <= position; i++) {
        iter += slot_size;
    }
    
    // 4. copy the right part of the node at the insertion position to a temp
    // allocate a new page temp to store the right part of the node at the insertion position
    char* temp = (char*)malloc(PageFile::PAGE_SIZE * sizeof(char));
    // initialize the new page values to zero
    memset(temp, 0, PageFile::PAGE_SIZE);
    
    bool insert_at_end = (position == getKeyCount());
    // if the new pair is not inserted at the end, copy the right part to temp
    if (insert_at_end == false) {
        // copy the right part to temp
        memcpy(temp, iter, slot_size * (getKeyCount() - position));
        // set the original right part to zero
        memset(iter, 0, slot_size * (getKeyCount() - position));
    }
    
    memcpy(iter, &key, sizeof(int));
    iter += sizeof(int);
    memcpy(iter, &pid, sizeof(PageId));
    iter += sizeof(PageId);
    
    // 5. if the new pair is not inserted at the end, copy back the temp part back after the new pair
    if (insert_at_end == false) {
        memcpy(iter, temp, slot_size * (getKeyCount() - position));
    }
    free(temp);
    
    // 6. increase the keyCount by 1
    // move the iter, point to the # key
    iter = &(buffer[0]);
    // increase the keyCount by 1
    int currKeyCount = getKeyCount();
    currKeyCount++;
    // store the new keyCount value to # key (the first 4 byte)
    memcpy(iter, &currKeyCount, sizeof(int));
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
    
    int nkeys = getKeyCount();
    int slot_size = sizeof(int) + sizeof(PageId);
    
    char* iter = &(buffer[0]);
    iter += sizeof(int); // # of keys
    iter += sizeof(PageId);
    
    int i = 0; // ith key
    while(i < nkeys / 2){
        iter += slot_size;
        i++;
    }
    
    int original_midkey;
    memcpy(&original_midkey, iter, sizeof(int)); // obtain the midkey
    char *tmp = (char*)malloc(1024 * sizeof(char)); // allocate a temp space
    memset(tmp, 0, 1024 * sizeof(char));
    
    if(key > original_midkey){
        // copy the second half past the midkey, to guarantee left has one more key than right
        iter += slot_size;
        if(nkeys % 2 == 0){ // even number of keys
            memcpy(tmp, iter, slot_size * (nkeys/2-1));
            memset(iter, 0, slot_size * (nkeys/2-1));
            
        }
        else{ // odd number of keys
            memcpy(tmp, iter, slot_size * (nkeys/2));
            memset(iter, 0, slot_size * (nkeys/2));
            
        }
        
        // update number of keys
        *(int *)buffer = nkeys/2+1;
        
        if(sibling.insert(key, pid) != 0) // insert into sibling
            return RC_FILE_WRITE_FAILED;
    }
    else{
        if(nkeys % 2 == 0){ // even number of keys
            memcpy(tmp, iter, slot_size * (nkeys/2));
            memset(iter, 0, slot_size * (nkeys/2));
            
        }
        else{ // odd number of keys
            memcpy(tmp, iter, slot_size * (nkeys/2+1));
            memset(iter, 0, slot_size * (nkeys/2+1));
        }
        
        // update number of keys
        *(int *)buffer = nkeys/2;
        
        if(insert(key, pid) != 0) // insert into the original node
            return RC_FILE_WRITE_FAILED;
    }
    
    // copy tmp to sibling node
    char* tempIter = tmp;
    while(tempIter){
        int k;
        PageId p;
        
        memcpy(&k, tempIter, sizeof(int));
        if(k == 0) break;
        tempIter += sizeof(int);
        memcpy(&p, tempIter, sizeof(PageId));
        tempIter += sizeof(PageId);
        
        if(sibling.insert(k, p) != 0)
            return RC_FILE_WRITE_FAILED;
    }
    
    // store the midkey
    iter -= slot_size;
    memcpy(&midKey, iter, sizeof(int));
    
    free(tmp);
    return 0;
}

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{
    int position = -1, realKey = -1;
    char* iter = &(buffer[0]);
    iter += sizeof(int);
    iter += sizeof(PageId);
    int slot_size = sizeof(int) + sizeof(PageId);
    int currKey = 0;
    bool found = false;
    for (int i = 0; i < getKeyCount(); i++) {
        // Read current key
        memcpy(&currKey, iter, sizeof(int));
        // found the insertion location (###########Could the input key be 0?)
        if (currKey >= searchKey) {
            position = i;
            realKey = currKey;
            found = true;
            break;
        }
        // move to the next pair
        iter += slot_size;
    }
    if (!found) {
        position = getKeyCount() - 1;
        realKey = currKey;
    }
    
    // First skip the initial key count and pageId in the leaf node structure
    iter = &(buffer[0]);
    
    // Skip previous keys
    for (int i = 0; i <= position; i++) {
        iter += slot_size;
    }
    
    if (searchKey < realKey) {
        iter -= sizeof(PageId);
    } else {
        iter += sizeof(int);
    }
    // Read entry values
    memcpy(&pid, iter, sizeof(PageId));
    
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
    char* iter = &(buffer[0]);
    int k = 1;
    memcpy(iter, &k, sizeof(int));
    iter += sizeof(int);
    
    memcpy(iter, &pid1, sizeof(PageId));
    iter += sizeof(PageId);
    memcpy(iter, &key, sizeof(int));
    iter += sizeof(int);
    memcpy(iter, &pid2, sizeof(PageId));
    
    return 0; 
}

