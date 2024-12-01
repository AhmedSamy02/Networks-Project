
#include <vector>
#include <string>
#include <fstream>
std::pair<std::string, std::string> coordinator_file_read()
{
    std::ifstream in("input_texts/coordinator.txt");
    std::string start_node = "", start_time = "";
    in >> start_node;
    in >> start_time;
    return std::pair<std::string, std::string>(start_node, start_time);
}
std::vector<std::pair<std::string, std::string>> read_file(bool node_number)
{
    std::vector<std::pair<std::string, std::string>> temp;
    std::string path = "input_texts/input";
    if (node_number)
        path += "1";
    else
        path += "0";
    path += ".txt";
    std::ifstream in(path);
    std::string text = "";
    while (std::getline(in, text))
    {
        auto p = std::pair<std::string, std::string>(text.substr(0, 4), text.substr(5));
        temp.push_back(p);
    }
    in.close();
    // [ 4-bits , line , 4-bits , line ,...... ]
    return temp;
}
int main(int argc, char const *argv[])
{
    // std::vector<std::pair<std::string, std::string>> temp = read_file(false);
    // for (size_t i = 0; i < temp.size(); i++)
    // {
    //     std::cout << temp[i].first << " : " << temp[i].second << std::endl;
    // }
    auto temp = coordinator_file_read();
    std::cout << temp.first << " : " << temp.second << std::endl;
    return 0;
}
