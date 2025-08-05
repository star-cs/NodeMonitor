#pragma once
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <chrono>

namespace monitor
{
class ReadFile
{
public:
    //接受文件名作为参数，并初始化文件输入流ifs_
    explicit ReadFile(const std::string &name) : ifs_(name) {}
    ~ReadFile() { ifs_.close(); }

    bool ReadLine(std::vector<std::string> *args)
    {
        std::string line;
        std::getline(ifs_, line);
        if (ifs_.eof() || line.empty()) {
            return false;
        }

        std::stringstream line_ss(line);
        while (!line_ss.eof()) {
            std::string word;
            line_ss >> word; //按空格分割字符串
            args->push_back(word);
        }
        return true;
    }

private:
    std::ifstream ifs_; //定义了一个私有的文件输入流对象，用于实际的文件读取操作
};

//duration对象表示一段时间间隔
//chrono 库中提供了一个表示时间点的类 time_point
inline double SteadyTimeSecond(const std::chrono::steady_clock::time_point &t1,
                               const std::chrono::steady_clock::time_point &t2)
{
    std::chrono::duration<double> time_second = t1 - t2;
    return time_second.count(); //count()是duration类的方法，返回时间间隔的数值部分
}

} // namespace monitor