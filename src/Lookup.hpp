#ifndef Lookup_HPP
#define Lookup_HPP
#include <vector>
#include <iostream>
using DVar = unsigned long long;
using Vertex = int;

class Lookup
{
    public:
        std::vector<int> node;
        std::vector<Vertex> lower;
        std::vector<Vertex> upper;
        int size;

        Lookup()
        {
            node.resize(0);
            lower.resize(0);
            upper.resize(0);
            size = 0;
        }
        void add(int n, Vertex l, Vertex u)
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
        Vertex get_index(int node)
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
        Vertex get_lower(int node)
        {
            return lower[get_index(node)];
        }
        Vertex get_upper(int node)
        {
            return upper[get_index(node)];
        }

        int get_node_for_value(Vertex value)
        {
            for (int i = 0; i < size;i++)
            {
                if (value >= lower[i] && value <= upper[i])
                {
                    return node[i];
                }
            }
            throw("node not found int the lookup table");
        }
        Lookup& operator=(const Lookup& other)
        {
            if (this != &other)
            {
                node = other.node;
                lower = other.lower;
                upper = other.upper;
                size = other.size;
            }
            return *this;
        }
};


#endif
