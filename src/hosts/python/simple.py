
#code is in W:\WinPython\WPy64-3771\python-3.7.7.amd64\Lib\site-packages\ipykernel\eventloops.py

@register_integration('asyncio')
def loop_asyncio(kernel):

    def runMegaHost():
        try:
            kernel.megaHost.runCycle()
        except AttributeError:
            import sys
            sys.path.append( "w:/root/megastructure/install/bin" )
            import python_host
            kernel.megaHost = python_host.Host( "w:/megatest/", "1001", "1002", "python_host.exe" )
            kernel.megaHost.runCycle()
        
    runMegaHost()
    
    '''Start a kernel with asyncio event loop support.'''
    import asyncio
    loop = asyncio.get_event_loop()
    # loop is already running (e.g. tornado 5), nothing left to do
    if loop.is_running():
        return

    if loop.is_closed():
        # main loop is closed, create a new one
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
    loop._should_close = False

    # pause eventloop when there's an event on a zmq socket
    def process_stream_events(stream):
        """fall back to main loop when there's a socket event"""
        if stream.flush(limit=1):
            loop.stop()
        
    for stream in kernel.shell_streams:
        fd = stream.getsockopt(zmq.FD)
        notifier = partial(process_stream_events, stream)
        loop.add_reader(fd, notifier)
        loop.call_soon(notifier)

    while True:
        error = None
        try:
            loop.run_forever()
        except KeyboardInterrupt:
            continue
        except Exception as e:
            error = e
        if loop._should_close:
            loop.close()
        if error is not None:
            raise error
        break
        
#C:\Users\eddeighton\AppData\Roaming\jupyter\runtime

import python_host
host = python_host.GetHost()
prog = host.getProgram()
root = prog.getRoot()
















