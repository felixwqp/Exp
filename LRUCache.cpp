
#include "../Leetcode/leetcode.h"

using Key = int;
using Val = int;
class LRUCache {
    
    
    struct Node{
        Key key = Key{0}; 
        Val val = Val{0};
        int ts = 0; 
        Node* prev = nullptr; 
        Node* next = nullptr;
        Node(int k, int v): key(k), val(v) {}
    };

    const int cache_size;
    Node* head;
    Node* tail;
    unordered_map<Key, Node*> node_map;
    int cur_size;
    int sum = 0;
    
    void _delink(Node* node){
        auto prev_ = node -> prev;
        auto next_ = node -> next;
        prev_ -> next = next_;
        next_ -> prev = prev_;
        node -> next = nullptr;
        node -> prev = nullptr;
        // cout << "delink: " << node -> key << endl;
    }
    
    void _insert_front(Node* node){
        auto old_prev_head = head -> prev;
        node -> prev = old_prev_head;
        old_prev_head -> next = node;
        node -> next = head;
        head -> prev = node;
        // cout << "insert front: " << node -> key << endl; 
        return;
        
    }
    
    bool _is_valid_time(int ts){
        // TODO
        // check with current time < threshold
        return true;
    }

    void _erase(){
        if(cur_size > cache_size || (cur_size > 0 && _is_valid_time(tail -> next -> ts))){
            auto last_node = tail -> next;
            // cout << "erase: " << last_node -> key << endl; 
            
            Val val = last_node -> key;
            _delink(last_node);
            node_map.erase(val);
            --cur_size;
            delete last_node;
        }
    }
    
public:
    LRUCache(int capacity)
        : cache_size(capacity)
        , cur_size(0)
    {
            head = new Node(0, 0);
            tail = new Node(0, 0);
            head -> next = nullptr;
            head -> prev = tail;
            tail -> next = head;
            tail -> prev = nullptr;
    }
    
    
    
    int get(int key) {
        auto iter = node_map.find(key);
        // cout << "get: " << key << endl;
        if(iter == node_map.end()){
            
            return -1;
        }else{
            // cout << "reach" << endl;
            auto move_node = iter -> second;
            _delink(move_node);
            _insert_front(move_node);
            return iter -> second -> val;
        }
    }
    
    
    void put(int key, int value) {
        Node* move_node = nullptr;
        auto iter = node_map.find(key);
        // cout << "put: " << key << endl;
        
        if(iter == node_map.end()){
            move_node = new Node(key, value);
            node_map.emplace(key, move_node);
            ++cur_size;
            sum += value;
        }else{
            move_node = iter -> second;
            sum += (0 - move_node -> val + value);
            move_node -> val = value;
            _delink(move_node);
        }
        _insert_front(move_node);
        
        _erase();
        
    }
};

/**
 * Your LRUCache object will be instantiated and called as such:
 * LRUCache* obj = new LRUCache(capacity);
 * int param_1 = obj->get(key);
 * obj->put(key,value);
 */

int main(){
int key = 20;
int val = 30;
LRUCache* obj = new LRUCache(2);
  cout <<  obj->get(key) << endl;
  obj->put(key,val);

  cout << obj->get(key) << endl;
    cout << "start !" << endl;
}


/*

unordered_map -> concurrenct_map


*/