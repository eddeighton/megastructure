import sys
import os
import IPython
from IPython.external.qt_for_kernel import QtCore, QtGui

sys.path.append( "w:/root/megastructure/install/bin" )

import python_hostd

# If we create a QApplication, keep a reference to it so that it doesn't get
# garbage collected.
_mega_appref = None
_mega_already_warned = False
_mega_callback = None

def mega_input_hook(context):
    global _mega_appref
    global _mega_callback
    app = QtCore.QCoreApplication.instance()
    if not app:
        if sys.platform == 'linux':
            if not os.environ.get('DISPLAY') \
                    and not os.environ.get('WAYLAND_DISPLAY'):
                import warnings
                global _mega_already_warned
                if not _mega_already_warned:
                    _mega_already_warned = True
                    warnings.warn(
                        'The DISPLAY or WAYLAND_DISPLAY environment variable is '
                        'not set or empty and Qt5 requires this environment '
                        'variable. Deactivate Qt5 code.'
                    )
                return
        QtCore.QCoreApplication.setAttribute(QtCore.Qt.AA_EnableHighDpiScaling)
        _mega_appref = app = QtGui.QApplication([" "])
    event_loop = QtCore.QEventLoop(app)

    if sys.platform == 'win32':
        # The QSocketNotifier method doesn't appear to work on Windows.
        # Use polling instead.
        timer = QtCore.QTimer()
        timer.timeout.connect(event_loop.quit)
        while not context.input_is_ready():
            if _mega_callback:
                _mega_callback()
            timer.start(50)  # 50 ms
            event_loop.exec_()
            timer.stop()
    else:
        # On POSIX platforms, we can use a file descriptor to quit the event
        # loop when there is input ready to read.
        notifier = QtCore.QSocketNotifier(context.fileno(),
                                          QtCore.QSocketNotifier.Read)
        try:
            # connect the callback we care about before we turn it on
            # lambda is necessary as PyQT inspect the function signature to know
            # what arguments to pass to. See https://github.com/ipython/ipython/pull/12355
            notifier.activated.connect(lambda: event_loop.exit())
            notifier.setEnabled(True)
            # only start the event loop we are not already flipped
            if not context.input_is_ready():
                event_loop.exec_()
        finally:
            notifier.setEnabled(False)

IPython.terminal.pt_inputhooks.register( "qt", mega_input_hook )

os.chdir( "w:/megatest/" )

host = python_hostd.Host( "w:/megatest/", "1001", "1002", "python_hostd.exe" )

cycle = 0
def runMegaCycle():
    global host
    global cycle
    cycle = cycle + 1
    host.runCycle()

_mega_callback = runMegaCycle

#ipython -i megastructure/src/src/python/startup.py
#%gui qt
#prog = host.getProgram()
#root = prog.getRoot()