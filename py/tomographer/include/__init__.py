
def get_include():
    """
    Return the directory to add to C++ include path in order to compile extensions using
    `tomographer`/`tomographerpy` headers.
    """
    import os.path
    
    return os.path.dirname(os.path.realpath(__file__))
