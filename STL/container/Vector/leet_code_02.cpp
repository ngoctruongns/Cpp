#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;
vector<int> findSubstring(string s, vector<string>& words)
{
    vector<int> result;
    unordered_map<string, int> wordsMap;

    // get init params
    int wordSize = words[0].length();
    int wordNum = words.size();
    int totalLength = wordSize * wordNum;
    // cout << "wordNum: "<< wordNum << " size: " << wordSize << " total length:" << totalLength <<  endl;

    // Get word map
    for ( string word : words)
    {
        wordsMap[word] += 1;
    }

    // print map
    for (auto udmap : wordsMap)
    {
        cout << udmap.first << ":" << udmap.second << ";";
    }

    // Check input
    if ( words.empty() || s.empty()) return result;
    if ((int)s.length() < totalLength) return result;

    // get sub string
    for (int id = 0; (id + totalLength) <= (int)s.length(); id++ )
    {
        string subStr = s.substr(id, totalLength);
        // Check substring
        auto wordsMapTemp = wordsMap;
        for (int idx=0; idx < wordNum; idx++)
        {
            string wordCheck = subStr.substr(idx*wordSize, wordSize);
            cout << wordCheck << endl;
            if ( wordsMapTemp.find(wordCheck) == wordsMapTemp.end())
            {
                // no word in map
                cout << "no word in map"<< endl;
                break;
            }
            else
            {
                // dec counter of word in map
                if (wordsMapTemp[wordCheck] <= 0)
                {
                    break;
                }
                else
                {
                    wordsMapTemp[wordCheck] -= 1;
                }

            }
            if (idx == (wordNum - 1))
            {
                result.push_back(id);
                cout << "result : " << id << endl;
            }
        }
    }
    return result;
}

int main(void)
{
    string s ="barfoofoobarthefoobarman";
    vector<string> words = {"bar","foo","the"};

    auto result = findSubstring(s, words);

    // print result
    for (int i : result)
    {
        cout << i << ", ";
    }
    return 0;
}