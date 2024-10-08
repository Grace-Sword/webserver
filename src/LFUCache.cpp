#include "LFUCache.h"
#include <iostream>
#include <vector>
#include <random>
#include <assert.h>

void KeyList::init(int freq) {
    freq_ = freq;
    // Dummyhead_ = tail_= new Node<Key>;
    Dummyhead_ = newElement<Node<Key>>();
    tail_ = Dummyhead_;
    Dummyhead_->setNext(nullptr);
}

// 删除整个链表
void KeyList::destory() {
    while(Dummyhead_ != nullptr) {
        key_node pre = Dummyhead_;
        Dummyhead_ = Dummyhead_->getNext();
        // delete pre;
        deleteElement(pre);
    }
}

int KeyList::getFreq() { return freq_; }

// 将节点添加到链表头部
void KeyList::add(key_node& node) {
    if(Dummyhead_->getNext() == nullptr) {
        // printf("set tail\n");

        tail_ = node;
        // printf("head_ is %p, tail_ is %p\n", Dummyhead_, tail_);
    }
    else {
        Dummyhead_->getNext()->setPre(node);
        // printf("this node is not the first\n");
    }
    node->setNext(Dummyhead_->getNext());
    node->setPre(Dummyhead_);
    Dummyhead_->setNext(node);

    assert(!isEmpty());
}

// 删除小链表中的节点
void KeyList::del(key_node& node) {
    node->getPre()->setNext(node->getNext());

    if(node->getNext() == nullptr) {
        tail_ = node->getPre();
    }
    else {
        node->getNext()->setPre(node->getPre());
    }
}

bool KeyList::isEmpty() {
    return Dummyhead_ == tail_;
}

key_node KeyList::getLast() {
    return tail_;
}
// LFUCache::LFUCache(int capacity) : capacity_(capacity) {
//  init();
// }
LFUCache::LFUCache(){
   // init();
}
LFUCache::~LFUCache() {
    while(Dummyhead_) {
        freq_node pre = Dummyhead_;
        Dummyhead_ = Dummyhead_->getNext();
        pre->getValue().destory();
        // delete pre;
        deleteElement(pre); 
    }
}
void LFUCache::Initialize(int capacity) {
    capacity_ = capacity;
    init();
}

void LFUCache::init() {
    // FIXME:缓存的容量动态变化
    
    // Dummyhead_ = new Node<KeyList>();
    Dummyhead_ = newElement<Node<KeyList>>();
    Dummyhead_->getValue().init(0);
    Dummyhead_->setNext(nullptr);
}
// 更新节点频度：
// 如果不存在下⼀个频度的链表，则增加⼀个
// 然后将当前节点放到下⼀个频度的链表的头位置
void LFUCache::addFreq(key_node& nowk, freq_node& nowf) {
    // printf("enter addFreq\n");
    freq_node nxt;
    // FIXME: 频数有可能有溢出
    if(nowf->getNext() == nullptr ||
        nowf->getNext()->getValue().getFreq() != nowf->getValue().getFreq() + 1) {
            // 新建一个下一频度的大链表,加到nowf后面
            // nxt = new Node<KeyList>();
            nxt = newElement<Node<KeyList>>();
            nxt->getValue().init(nowf->getValue().getFreq() + 1);
            if(nowf->getNext() != nullptr)
                nowf->getNext()->setPre(nxt);
            nxt->setNext(nowf->getNext());
            nowf->setNext(nxt);
            nxt->setPre(nowf);
            // printf("the prefreq is %d, and the cur freq is %d\n", nowf->getValue().getFreq(), nxt->getValue().getFreq());
    }
    else {
        nxt = nowf->getNext();
    }
    fmap_[nowk->getValue().key_] = nxt;// 更新频度映射到新创建的大链表
    // 将其从上一频度的小链表删除
    // 然后加到下一频度的小链表中
    if(nowf != Dummyhead_) {
        nowf->getValue().del(nowk);
    }
    nxt->getValue().add(nowk);

    //printf("the freq now is %d\n", nxt->getValue().getFreq());
    assert(!nxt->getValue().isEmpty());

    // 如果该频度的小链表已经空了
    if(nowf != Dummyhead_ && nowf->getValue().isEmpty())
        del(nowf);
}

//用于查找缓存中的键值对
bool LFUCache::get(string& key, string& val) {
    if(!capacity_) return false;
        mutex_.lock();//修改加锁
    if(fmap_.find(key) != fmap_.end()) {
        // 缓存命中
        key_node nowk = kmap_[key];
        freq_node nowf = fmap_[key];
        //根据具体场景，需要修改
        // val += nowk->getValue().value_;// 将缓存中的值添加到返回的 val 中
        val = nowk->getValue().value_;
        addFreq(nowk, nowf);
        mutex_.unlock();  // 手动解锁
        return true;
    }
    // 未命中
    mutex_.unlock();  // 手动解锁
    return false;
}

//将新的键值对添加进缓存
void LFUCache::set(string& key, string& val) {
   if(!capacity_) return;
   // printf("kmapsize = %d capacity = %d\n", kmap_.size(), capacity_);
   mutex_.lock();//修改加锁
   // 缓存满了
   // 从频度最⼩的⼩链表中的节点中删除最后⼀个节点（⼩链表中的删除符合LRU）
   if(kmap_.size() == capacity_) {
      // printf("need to delete a Node\n");
      freq_node head = Dummyhead_->getNext();
      if (!head) {
            mutex_.unlock();
            return;
      }
      key_node last = head->getValue().getLast();// 获取最小频率链表中的最后一个节点
      if(last){
         head->getValue().del(last);
         kmap_.erase(last->getValue().key_);
         fmap_.erase(last->getValue().key_);
         // delete last;
         deleteElement(last);
         // 如果频度最⼩的链表已经没有节点，就删除
         if(head->getValue().isEmpty()) {
            del(head);
         }
      }
   }
   // key_node nowk = new Node<Key>();
   // 使⽤内存池
   key_node nowk = newElement<Node<Key>>();
   if (!nowk) {
        mutex_.unlock();
        return; // 如果内存池分配失败，返回
    }
   nowk->getValue().key_ = key;
   nowk->getValue().value_ = val;

   addFreq(nowk, Dummyhead_);
   kmap_[key] = nowk;
   fmap_[key] = Dummyhead_->getNext();

   mutex_.unlock();  // 解锁
}

void LFUCache::del(freq_node& node) {
    node->getPre()->setNext(node->getNext());
    if(node->getNext() != nullptr) {
        node->getNext()->setPre(node->getPre());
    }
    node->getValue().destory();
    // delete node;
    deleteElement(node);
}

LFUCache& LFUCache::GetInstance() {
    static LFUCache cache;
    return cache;
}
