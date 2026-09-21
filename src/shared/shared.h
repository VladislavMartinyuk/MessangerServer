#ifndef SHARED_H
#define SHARED_H

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <string>

inline std::string generateUuid() {
    static thread_local boost::uuids::random_generator generator;
    return boost::uuids::to_string(generator());
}

#endif // SHARED_H
