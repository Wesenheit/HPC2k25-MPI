#ifndef Lookup_HPP
#define Lookup_HPP
#include <vector>
class Lookup
{
    std::vector<int> node;
    std::vector<int> lower;
    std::vector<int> upper;
    int size;
    public:
        Lookup()
        {
            node.resize(0);
            lower.resize(0);
            upper.resize(0);
            size = 0;
        }
        void add(int n, int l, int u)
        {
            node.push_back(n);
            lower.push_back(l);
            upper.push_back(u);
            size++;
        }
        int leng()
        {
            return size;
        }
        int get_index(int node) const
        {
            for (int i = 0; i < this->node.size(); i++)
            {
                if (this->node[i] == node)
                {
                    return i;
                }
            }
            return -1;
        }
        int get_lower(int node)
        {
            return lower[get_index(node)];
        }
        int get_upper(int node)
        {
            return upper[get_index(node)];
        }

        int get_node_for_value(int value)
        {
            for (int i = 0; i < size;i++)
            {
                if (value > lower[i] && value < upper[i])
                {
                    return node[i];
                }
            }
            return -1;
        }

};


#endif