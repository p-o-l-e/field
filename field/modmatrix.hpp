#pragma once
#include <vector>
#include "curves.hpp"
#include "containers.hpp"
#include "primitives.hpp"
#include <iostream>
using std::vector;
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
namespace cell {


//////////////////////////////////////////////////////////////////////////////////
// PatchCord /////////////////////////////////////////////////////////////////////
struct patchcord
{
        LR<float>* data;       // Rendered spline
        LR<float>  spline[4];  // Control points
        const int   segments = 4;   // # Segments
        const int   iterations;     // Precision
        void process();             // Fill spline data
        patchcord(int);
       ~patchcord();
};


//////////////////////////////////////////////////////////////////////////////////
// Socket Data ///////////////////////////////////////////////////////////////////
class socket
{
    public:
        square<uint> bounds {0, 0, 0};
        patchcord cord;
        uint    id    = 0;              // Identifier
        bool    route = 0;              // 0: Out - 1: In
        bool    on = false;             // Is connected ?
        socket* to = nullptr;
        const uint* w;
        const uint* h;
        void collapse();                // Collapse to centre
        void drag(const float, const float);    // Drag
        socket(int);  // cord(j) {}
       ~socket();
};

//////////////////////////////////////////////////////////////////////////////////
// Patchbay //////////////////////////////////////////////////////////////////////
class patchbay
{
    public:
        socket* io;
        frame<uint> canvas;
        socket* src = nullptr;
        socket* dst = nullptr;
        const int nodes;              // Number of sockets
        void draw();
        int  down_test(float, float);
        bool up_test(float, float);
        void set_socket(int, int, int, uint, uint, bool);
        void drag(float x, float y);
        void connect(socket*, socket*);
        void disconnect(socket*, socket*);
        patchbay(int, int, int);
       ~patchbay();
};

//////////////////////////////////////////////////////////////////////////////////
// Modmatrix /////////////////////////////////////////////////////////////////////
// class modmatrix
// {
//     private:
//     public:
//         frame<float> data;
//         modmatrix(uint, uint);
//        ~modmatrix();
// };

// modmatrix::modmatrix(uint ins, uint outs): data(ins, outs)
// {
// }

// modmatrix::~modmatrix()
// {
// }



};
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////