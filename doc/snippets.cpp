#include "sss/host.h"
#include "sss/server.h"
#include "sss/stream.h"

int
main()
{
    //! [Creating a host]
    host_ptr my_host(host::create());
    //! [Creating a host]
}

//! [Accepting a connection]
void
some_accept_func(sss::server_ptr server)
{
    while (auto stream = server->accept()) {
        // Do something with a new stream.
    }
}
//! [Accepting a connection]
