template <class T> bool itemexists( std::vector<T> vec, T item ) {
    return std::find( vec.begin(), vec.end(), item ) != vec.end();
}
