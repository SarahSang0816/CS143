/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 * Nov 21th, edited by Qi Sang
 */

#include "BTreeIndex.h"
#include "BTreeNode.h"

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
    /* Initially, use a leafNode as both a leaf and root
     * PageId of the root node is set to 1, initial root content is stored in PageId 1
     * PageId 0 is reserved to store the treeHeight, rootPid, PageIdCount
     * other derived node later would be stored in the following pages
     */
    rootPid = 1;
    BTLeafNode root;
    root.write(rootPid, pf);
    
    /* there is a empty root node from the begining */
    treeHeight = 1;
    PageIdCount = 1;
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
    this->mode = mode;
    /* get the file descriptor (fd), and (last page id + 1) of the file (epid)
     * In simple words, when you open a file,
     * the operating system creates an entry to represent that file
     * and store the information about that opened file.
     * So it is just an integer number that uniquely represents an opened file in operating system.
     */
    pf.open(indexname, mode);
    
    if (mode == 'r') {
        /* read the first page of the index file, and set rootPid and treeHeight */
        char buffer[PageFile::PAGE_SIZE];
        pf.read(0, buffer);
        
        char* iter = &(buffer[0]);
        memcpy(&rootPid, iter, sizeof(PageId));
        iter += sizeof(PageId);
        memcpy(&treeHeight, iter, sizeof(int));
        iter += sizeof(int);
        memcpy(&PageIdCount, iter, sizeof(int));
    }
    
    return 0;
}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close()
{
    /* if the index file was read, not be writen, close the opened file directily
     * if the index file was written, modified, write the content back to the disk before close the file
     */
    if (mode == 'w') {
        char buffer[PageFile::PAGE_SIZE];
        char* iter = &(buffer[0]);
        memset(iter, 0, PageFile::PAGE_SIZE);
        
        memcpy(iter, &rootPid, sizeof(PageId));
        iter += sizeof(PageId);
        memcpy(iter, &treeHeight, sizeof(PageId));
        iter += sizeof(int);
        memcpy(&PageIdCount, iter, sizeof(int));
        
        pf.write(0, buffer);
    }
    return pf.close();
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid)
{
    /* insert into the correct location into the leaf node and update the parent node
     */
    IndexCursor cursor;
    
    /* if no tuple has been inserted into the B+ tree index,
     * the B+ index tree only has a empty root
     * treeHeight is 1
     */
    BTLeafNode root;
    root.read(rootPid, pf);
    if (treeHeight == 1) {
        /* if initial root node is not full, insert and then write back to disk */
        if (root.insert(key,rid) == 0) {
            root.write(rootPid, pf);
        }
        /* if initial root node is full,
         * call insertAndSplit, get the siblingKey
         * create a new root node, link the root node to leaf node
         * treeHeight++
         */
        else {
            /* insert the new pair into the new sibling leaf node */
            BTLeafNode sibling;
            int siblingKey = 0;
            root.insertAndSplit(key, rid, sibling, siblingKey);
            int siblingPid = ++PageIdCount;
            /* link two leaf node */
            root.setNextNodePtr(siblingPid);
            /* create a new root */
            BTNonLeafNode newRoot;
            int newRootPid = ++PageIdCount;
            newRoot.initializeRoot(rootPid, siblingKey, siblingPid);
            /* write all modified node back to disk */
            root.write(rootPid, pf);
            sibling.write(siblingPid, pf);
            newRoot.write(newRootPid, pf);
            /* update rootPid and treeHeight */
            rootPid = newRootPid;
            treeHeight++;
        }
        
    }
    /* if treeHeight is greater than one */
    else {
        /* find the position to insert the new pair */
        IndexCursor cursor;
        locate(key, cursor);
        /* insert the new pair into the leaf level */
        BTLeafNode currLeafNode;
        currLeafNode.read(cursor.pid, pf);
        /* if the leaf is not full, insert and then write back to disk */
        if (currLeafNode.insert(key, rid) == 0) {
            currLeafNode.write(cursor.pid, pf);
        }
        /* if the leaf is full, insert and split, update parents */
        else {
            BTLeafNode siblingLeaf;
            BTNonLeafNode siblingNonLeaf;
            int siblingKey = -1, siblingPid = -1;
            /* get locate traverse path */
            vector<PageId> parent = cursor.parent;
            for (int i = parent.size(); i >= 0; i--) {
                // Initial leaf level
                if (i == parent.size()) {
                    //                    cout << "spliting leaf with key: " << key << endl;
                    currLeafNode.insertAndSplit(key, rid, siblingLeaf, siblingKey);
                    /* allocate next avaiable Pid to the new sibling leaf node */
                    siblingPid = ++PageIdCount;
                    //                    cout << " siblingKey is " << siblingKey << " siblingPid is " << siblingPid << endl;
                    //                    cout << endl;
                    /* link with the new sibling leaf node */
                    int prevSiblingPid = currLeafNode.getNextNodePtr();
                    siblingLeaf.setNextNodePtr(prevSiblingPid);
                    currLeafNode.setNextNodePtr(siblingPid);
                    currLeafNode.write(cursor.pid, pf);
                    siblingLeaf.write(siblingPid, pf);
                }
                // Non-leaf level (include root level)
                else {
                    BTNonLeafNode currNode;
                    currNode.read(parent[i], pf);
                    if (currNode.insert(siblingKey, siblingPid) == 0) {
                        currNode.write(parent[i], pf);
                        break;
                    }
                    else {
                        //                        cout << "spliting non leaf" << endl;
                        currNode.insertAndSplit(siblingKey, siblingPid, siblingNonLeaf, siblingKey);
                        siblingPid = ++PageIdCount;
                        currNode.write(parent[i], pf);
                        siblingNonLeaf.write(siblingPid, pf);
                        
                        if (i == 0) {
                            BTNonLeafNode newRoot;
                            int newRootPid = ++PageIdCount;
                            newRoot.initializeRoot(rootPid, siblingKey, siblingPid);
                            newRoot.write(newRootPid, pf);
                            /* update rootPid and treeHeight */
                            rootPid = newRootPid;
                            treeHeight++;
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}

/**
 * Run the standard B+Tree key search algorithm and identify the
 * leaf node where searchKey may exist. If an index entry with
 * searchKey exists in the leaf node, set IndexCursor to its location
 * (i.e., IndexCursor.pid = PageId of the leaf node, and
 * IndexCursor.eid = the searchKey index entry number.) and return 0.
 * If not, set IndexCursor.pid = PageId of the leaf node and
 * IndexCursor.eid = the index entry immediately after the largest
 * index key that is smaller than searchKey, and return the error
 * code RC_NO_SUCH_RECORD.
 * Using the returned "IndexCursor", you will have to call readForward()
 * to retrieve the actual (key, rid) pair from the index.
 * @param key[IN] the key to find
 * @param cursor[OUT] the cursor pointing to the index entry with
 *                    searchKey or immediately behind the largest key
 *                    smaller than searchKey.
 * @return 0 if searchKey is found. Othewise an error code
 */
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor)
{
    int currentPid = rootPid;
    int eid;
    for(int level = 1; level < treeHeight; level++){
        BTNonLeafNode node;
        cursor.parent.push_back(currentPid);
        if(node.read(currentPid, pf) != 0)
            return RC_FILE_READ_FAILED;
        
        node.locateChildPtr(searchKey, currentPid);
    }
    
    BTLeafNode node; // there is only one leaf node
    if(node.read(currentPid, pf) != 0)
        return RC_FILE_READ_FAILED;
    RC rc = node.locate(searchKey, eid);
    cursor.pid = currentPid;
    cursor.eid = eid;
    
    return rc;
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
    
    BTLeafNode node;
    RC rc = node.read(cursor.pid, pf);
    if(rc != 0)
        return rc;
    if (cursor.eid == node.getKeyCount()) {
        if (node.getNextNodePtr() == 0) {
            key = -1;
            rid.pid = -1;
            return RC_END_OF_TREE;
        }
        cursor.pid = node.getNextNodePtr();
        cursor.eid = 0;
        RC rc = node.read(cursor.pid, pf);
        if(rc != 0)
            return rc;
    }
    rc = node.readEntry(cursor.eid, key, rid);
    if(rc != 0)
        return rc;
    
    cursor.eid++;
    
    
    return 0;
}
