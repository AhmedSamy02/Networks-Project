#include <iostream>
using namespace std;
const char FLAG = '$';
const char ESC = '\\';
string framing(string s)
{
    string s1 = "";
    s1 += FLAG;
    for (size_t i = 0; i < s.length(); i++)
    {
        if (s[i] == FLAG || s[i] == ESC)
        {
            s1 += ESC;
        }
        s1 += s[i];
    }
    s1 += FLAG;
    return s1;
}

string deframing(string s){
    string s1 = "";
    for (size_t i = 1; i < s.length()-1; i++)
    {
        if (s[i] == ESC)
        {
            i++;
        }
        s1 += s[i];
    }
    return s1;
}

int main(int argc, char const *argv[])
{
    string s;
    cin >> s;
    auto s1 = framing(s);
    cout << framing(s) << endl;
    auto s2 = deframing(s1);
    cout << deframing(s1) << endl;
    return 0;
}

