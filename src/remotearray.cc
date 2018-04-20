#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <string>
#include <iostream>

#ifdef _WIN32
#include <windows.h> 
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

namespace py = pybind11;

template<class T>
class RemoteArray : public std::vector<T> {
  std::vector<T> _remote;
  std::string _ip;
  int _port;
  int sockfd;
public:
  explicit RemoteArray(std::string ip, int port, size_t n) :
    std::vector<T>(n), _ip(ip), _port(port), _remote(n) {
      init_network();
    };

  RemoteArray(std::string ip, int port, size_t n, const T& val) :
    std::vector<T>(n, val), _ip(ip), _port(port), _remote(n, val) {
      init_network();
  };

private:
#if _WIN32
	WSADATA wsaData;
#endif
	void init_network() {
#if _WIN32
		static bool startedup = false;
		if (!startedup) {
			int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
			if (iResult != 0) {
				printf("WSAStartup failed with error: %d\n", iResult);
				exit(1);
			}
			startedup = true;
		}
#endif
    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serveraddr;
    if (sockfd < 0) {
#if _WIN32
		printf("found windows error %d\n", WSAGetLastError());
#endif
      perror("Error opening socket");
      exit(1);
    }

    /* gethostbyname: get the server's DNS entry */
    struct hostent *server = gethostbyname(_ip.c_str());
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", _ip.c_str());
        exit(1);
    }

    /* build the server's Internet address */
    memset((char *) &serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    memcpy((char *)&serveraddr.sin_addr.s_addr, (const char *)server->h_addr, server->h_length);
    serveraddr.sin_port = htons(_port);

    /* connect: create a connection with the server */
    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)  {
#if _WIN32
		printf("found windows error %d\n", WSAGetLastError());
#endif
		perror("Error connecting");
      exit(1);
    }
    flush(true);
  }

  void send_data(void* dat, size_t sz) {
    int n = send(sockfd, (const char*)dat, sz, 0);
    if (n < 0) {
#if _WIN32
		printf("found windows error %d\n", WSAGetLastError());
#endif
		perror("Error writing");
		close();
		init_network();
    }
  }

public:
  void close() {
    if (sockfd > 0) {
#if _WIN32
		::closesocket(sockfd);
#else
		::close(sockfd);
#endif
		sockfd = -1;
    }
  }

  void flush(bool force=false) {
    auto sz = this->size();

    if (force) {
      struct {
        uint16_t offset;
        uint16_t length;
      } info;
      info.offset = 0;
      info.length = sz;
      send_data(&info, sizeof(info));
      send_data(this->data(), sizeof(T)*sz);
    } else {
      //TODO allow smaller, more efficient chunking
      struct {
        uint16_t offset;
        uint16_t length;
      } info;
      info.offset = 0;
      info.length = sz;
      send_data(&info, sizeof(info));
      send_data(this->data(), sizeof(T)*sz);
    }

    for(unsigned i=0; i<sz; i++) {
      _remote[i] = std::vector<T>::operator[](i);
    }
  }

};

typedef uint32_t ws2811_led_t;                   //< 0xWWRRGGBB

PYBIND11_MODULE(remotearray, m) {

  py::class_<RemoteArray<ws2811_led_t>>(m,"RemoteLED", py::buffer_protocol())
	.def_buffer([](RemoteArray<ws2811_led_t> &m) -> py::buffer_info {
		  return py::buffer_info(
			  m.data(),                               /* Pointer to buffer */
			  sizeof(ws2811_led_t),                          /* Size of one scalar */
			  py::format_descriptor<ws2811_led_t>::format(), /* Python struct-style format descriptor */
			  1,                                      /* Number of dimensions */
			  { m.size() },                 /* Buffer dimensions */
			  { sizeof(ws2811_led_t) }             /* Strides (in bytes) for each index */
		);
	})
    .def(py::init<std::string, int, size_t>())
    .def(py::init<std::string, int, size_t, ws2811_led_t>())
    //.def(py::init<const std::vector<float>&>())
    .def("__getitem__", [=](const RemoteArray<ws2811_led_t>& d, int i) {
        if (i >= d.size()) throw py::index_error();
        return d[i];
    })
    .def("__setitem__", [=](RemoteArray<ws2811_led_t>& d, int i, ws2811_led_t value) {
        if (i >= d.size()) throw py::index_error();
        return d[i] = value;
    })
	.def("__setitem__", [](RemoteArray<ws2811_led_t> &s, py::slice slice, py::array_t<ws2811_led_t> &value) {
		size_t start, stop, step, slicelength;
		if (!slice.compute(s.size(), &start, &stop, &step, &slicelength))
			throw py::error_already_set();
		if (slicelength != value.size())
			throw std::runtime_error("Left and right hand size of slice assignment have different sizes!");
		const auto buf = value.request();

		if (buf.ndim != 1)
			throw std::runtime_error("Number of dimensions must be one");

		const auto ptr = (const ws2811_led_t*)buf.ptr;

		for (size_t i = 0; i < slicelength; ++i) {
			s[start] = ptr[i];
			start += step;
		}
	})
	.def("__iter__", [](const RemoteArray<ws2811_led_t>& s) { return py::make_iterator(s.begin(), s.end()); },
                     py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */)
    .def("__len__", &RemoteArray<ws2811_led_t>::size)
    .def("flush", &RemoteArray<ws2811_led_t>::flush, py::arg("force") = false)
    .def("close", &RemoteArray<ws2811_led_t>::close)
    ;
}
