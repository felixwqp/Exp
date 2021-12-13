//
// Created by 王起鹏 on 12/12/21.
//

#ifndef PLAYGROUND_FILEHANDLER_H
#define PLAYGROUND_FILEHANDLER_H
#include <string>
#include <iostream>
#include <fstream>


class FileHandler{
    enum class WRITE_MODE{
        APPEND,
        WRITE
    };

public:
    FileHandler();
    /*
     * throw exceptions???
     *
     * */
    bool simple_delete(std::string object_path);

    bool delete_files(std::string directory, std::string reg_pattern);

    std::string read_file_lines(std::string file_path, int num_start_line, int max_line_len, int num_read_lines);

    int count_str(std::string path, int num_start_line, int max_line_len, std::string search_word);

    int write_file(std::string path, std::string content,  WRITE_MODE mode = WRITE_MODE::APPEND);

    std::vector<std::string> ls_folder(std::string path);

    // obsider lock


};


void stream_test(){
    std::string input_file_str = "input.data";
    std::string output_file_str = "output.data";
    std::ifstream input_file(input_file_str);
    std::ofstream output_file(output_file_str);



}

/*
 * handiest form of disk -> text file {ascii}
 * input stream (ifstream): disk -> ->
 *
 * */

#endif //PLAYGROUND_FILEHANDLER_H
