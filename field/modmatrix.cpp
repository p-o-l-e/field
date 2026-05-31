#include "modmatrix.hpp"
namespace cell {

//////////////////////////////////////////////////////////////////////////////////
// PatchCord /////////////////////////////////////////////////////////////////////
void patchcord::process()
{
    LR<float> car;
    float inc = 1.04/(float)iterations;
    float t = 0.0;
    for(int i = 0; i < iterations; i++)
    {
        car = interpolateBezier(spline[0], spline[1], spline[2], spline[3], t);
        data[i].l = car.l;
        data[i].r = car.r;
        t += inc;
    }
}

patchcord::patchcord(int j = 8): iterations(j)
{
    data = new LR<float>[iterations];
}

patchcord::~patchcord()
{
    delete[] data;
}

//////////////////////////////////////////////////////////////////////////////////
// Socket ////////////////////////////////////////////////////////////////////////
void socket::collapse()
{
    for(int i = 0; i < cord.segments; i++)
    {
        cord.spline[i].l = bounds.xCentre;
        cord.spline[i].r = bounds.yCentre;
    }
    cord.process();
}


void socket::drag(const float x, const float y)
{          
    auto xe = x, ye = y;

    if(xe > *w) xe = *w;
    if(xe < 0) xe = 0;
    if(ye > *h) ye = *h;
    if(ye < 0) ye = 0;

    
    float xo = cord.spline[0].l;
    float yo = cord.spline[0].r;

    cord.spline[1].l = (xe - xo) * 0.6 + xo;
    cord.spline[2].l = (xe - xo) * 0.4 + xo;

    if(ye < yo)
    {
        cord.spline[1].r = abs(yo - ye) * 0.8333 + ye;
        cord.spline[2].r = abs(yo - ye) * 0.1666 + ye;
    }

    if(ye > yo)
    {
        cord.spline[1].r = abs(yo - ye) * 0.1666 + yo;
        cord.spline[2].r = abs(yo - ye) * 0.8333 + yo;
    }

    cord.spline[3].l = xe;
    cord.spline[3].r = ye;

    cord.process();
}

socket::socket(int n = 32): cord(n)
{
    collapse();
}

socket::~socket() 
{

}


//////////////////////////////////////////////////////////////////////////////////
// Patchbay //////////////////////////////////////////////////////////////////////
void patchbay::connect(socket* I, socket* O)
{
    I->drag(O->bounds.xCentre, O->bounds.yCentre);
    I->on = true;
    O->on = true;
    I->to = O;
    O->to = I;
}

void patchbay::disconnect(socket* I, socket* O)
{
    I->on = false;
    O->on = false;
    I->to = nullptr;
    O->to = nullptr;
}

void patchbay::draw()
{
    for(int i = 0; i < nodes; i++)
    {
        drawSquareFilled<uint>(&canvas, io[i].bounds, io[i].id);
    }
}

int patchbay::down_test(float x, float y)
{
    auto stamp = canvas.get(x, y); 

    for(int i = 0; i < nodes; i++)
    {
        if((stamp == io[i].id) && stamp) // Socket is hitted
        {
            std::cout<<"156:Button down on socket: "<<std::hex<<io[i].id<<"\n";
            // Temporary source is free /////////////////////////////////////
            if(io[i].on)   // Already connected?
            {
                src = io[i].to;
                src->drag(io[i].bounds.xCentre, io[i].bounds.yCentre);
                //src->collapse();
                io[i].collapse();
                std::cout<<"164:Button is down - already connected!\n";
                disconnect(&io[i], io[i].to);
                // src = nullptr;
                return src->route? 1 : -1;
            }
            else
            {
                src = &io[i];
                std::cout<<"171:New source armed!\n";
                src->collapse();
                return src->route? 1 : -1;
            }
        }
    }
    // Void hitted
    std::cout<<"178:Hit at void \n";
    if(src!=nullptr) 
    {
        src->collapse();
    }
    src = nullptr;
    return 0;
}



bool patchbay::up_test(float x, float y)
{
    auto stamp = canvas.get(x, y); 

    for(int i = 0; i < nodes; i++)
    {
        if((stamp == io[i].id) && stamp) // Socket is hitted
        {
            std::cout<<"197:Button up at socket: "<<std::hex<<io[i].id<<"\n";
            if(src == &io[i]) // Same socket is hitted
            {
                src->collapse();
                src = nullptr;
                std::cout<<"202:Self Hit\n";
                return false;
            }

            if(src->route == io[i].route) // Same socket is hitted
            {
                src->collapse();
                src = nullptr;
                std::cout<<"177:Wrong route!\n";
                return false;
            }

            else // Other socket is hitted
            {
                // Temprary source is free //////////////
                if(src == nullptr) 
                {
                    std::cout<<"211:No Source ! \n";
                    return false;
                }
                else
                {

                    if(io[i].on)   // Already connected?
                    {
                        if(src->route != io[i].route)
                        {
                            std::cout<<"188:Src route:\t"<<src->route<<"\n";
                            std::cout<<"189:Dst route:\t"<<io[i].route<<"\n";
                            io[i].collapse();
                            io[i].to->collapse();
                            disconnect(&io[i], io[i].to);
                            //src->collapse();
                            std::cout<<"223:Already connected!\n";
                            connect(src, &io[i]);
                            src = nullptr;
                            return true;
                        }
                        src = nullptr;
                        return false;
                    }
                    else
                    {
                        if(src->route != io[i].route)
                        {
                            std::cout<<"203:Src route:\t"<<src->route<<"\n";
                            std::cout<<"203:Dst route:\t"<<io[i].route<<"\n";
                            std::cout<<"230:Connected!\n";
                            connect(src, &io[i]);
                            src = nullptr;
                            return true;
                        }
                        src = nullptr;
                        return false;
                    }
                }
            }
        }
    }

    // Void hitted
    std::cout<<"241:Hit at void \n";
    if(src!=nullptr) 
    {
        src->collapse();
    }
    
    src = nullptr;
    
    return false;
}

void patchbay::drag(float x, float y)
{
    if(src != nullptr)
    {
        src->drag(x, y);
    }
}


patchbay::patchbay(int w, int h, int n): canvas(w, h), nodes(n)
{
    std::cout<<"Width : "<<canvas.width<<"\n";
    std::cout<<"Height: "<<canvas.height<<"\n";
    std::cout<<"Nodes : "<<nodes<<"\n";
    io = new socket[nodes];
    for(int i = 0; i < nodes; i++)
    {
        io[i].w  = &canvas.width;
        io[i].h = &canvas.height;
    }
}

patchbay::~patchbay()
{
    src = nullptr;
    delete[] io;
}

void patchbay::set_socket(int xo, int yo, int radius, uint pos, uint id, bool route)
{
    if(pos<nodes)
    {
        io[pos].bounds.xCentre = xo;
        io[pos].bounds.yCentre = yo;
        io[pos].bounds.radius  = radius;
        io[pos].id = id;
        io[pos].route = route;
        io[pos].collapse();
    }
}



};
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
