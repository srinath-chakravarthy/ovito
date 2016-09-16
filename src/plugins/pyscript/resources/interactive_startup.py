# This script is executed when running 'ovitos' without arguments.
# It starts up the IPython interactive prompt (if available).
if __name__ == '__main__':
    import sys
    try:
        import IPython
        print("This is OVITO's interactive IPython interpreter. Use quit() or Ctrl-D to exit.")
        IPython.start_ipython(['--nosep','--no-confirm-exit','--no-banner','--profile=ovito','-c','import ovito','-i'])
        sys.exit()
    except ImportError:
        pass
    import ovito
    import code
    code.interact(banner="This is OVITO's interactive Python interpreter. Use quit() or Ctrl-D to exit.")
