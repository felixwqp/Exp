//
// Created by 王起鹏 on 1/20/22.
//

#ifndef PLAYGROUND_CONNECT4_H
#define PLAYGROUND_CONNECT4_H

#include <iostream>
#include <vector>
#include <string>


using std::vector;



class Connect4Board{
    static const int len = 7;
    enum class PlayerToken{
        Empty,
        P1,
        P2
    };

    vector<vector<int>> dirs = {{1,0}, {0,1}, {1,1}, {-1,1}};

    vector<vector<PlayerToken>> board;
    PlayerToken cur_idx = PlayerToken::Empty;
    vector<int> row_idx_to_place;

public:
    Connect4Board()
    : board(len, vector<PlayerToken>(len, PlayerToken::Empty))
    , row_idx_to_place(len, 0){

    }

    bool place(int col, PlayerToken token){
        if(col < 0 || col >= len){
            throw std::runtime_error("Col Idx out of bound");
        }
        if(row_idx_to_place[col] >= len){
            throw std::runtime_error("No more space on current col");
        }
        // check on the token -> take turn
        const int row_idx = row_idx_to_place[col]++;
        assert(board[row_idx][col] == PlayerToken::Empty);
        board[row_idx][col] = token;
        return check_win(row_idx, col);
    }

    int check_dis(int row, int col, const vector<int>& dir){
        int dis = 0;
        while(row + (dis + 1) * dir[0] >= 0 && row + (dis + 1) * dir[0] < len
            && col + (dis + 1) * dir[1] >= 0 && col + (dis + 1) * dir[1] < len
            && board[row + (dis + 1) * dir[0]][col + (dis + 1) * dir[1]] == board[row][col] ){
            ++dis;
        }
        return dis;
    }

    bool check_win(int row, int col){
        for(const auto& dir: dirs){
            auto pos_dir(dir);
            auto neg_dir(dir);
            int dis = 1 + check_dis(row, col, pos_dir) + check_dis(row, col, neg_dir);
            if(dis >= 4) return true;
        }
        return false;
    }


};






#endif //PLAYGROUND_CONNECT4_H
