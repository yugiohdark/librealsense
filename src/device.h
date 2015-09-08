#pragma once
#ifndef LIBREALSENSE_DEVICE_H
#define LIBREALSENSE_DEVICE_H

#include "uvc.h"

#include <mutex>

struct rs_device
{
protected:
    class subdevice_handle;

    // Provides a stream of images to the user, and stores information about the current mode
    class stream_buffer
    {
        struct frame
        {
            std::vector<uint8_t>                    pixels;
            int                                     number;

                                                    frame() : number() {}

            void                                    swap(frame & r) { pixels.swap(r.pixels); std::swap(number, r.number); }
        };

        rsimpl::stream_mode                         mode;
        frame                                       front, middle, back;
        std::mutex                                  mutex;
        volatile bool                               updated = false;
    public:
        const rsimpl::stream_mode &                 get_mode() const { return mode; }
        const void *                                get_image() const { return front.pixels.data(); }
        int                                         get_frame_number() const { return front.number; }

        void                                        set_mode(const rsimpl::stream_mode & mode);
        bool                                        update_image();

        void                                        set_back_number(int number) { back.number = number; }
        void *                                      get_back_image() { return back.pixels.data(); }
        void                                        swap_back();
    };

    rsimpl::uvc::device                             device;
    const rsimpl::static_device_info                device_info;

    rsimpl::stream_request                          requests[RS_STREAM_COUNT];  // Indexed by RS_DEPTH, RS_COLOR, ...
    std::shared_ptr<stream_buffer>                  streams[RS_STREAM_COUNT];   // Indexed by RS_DEPTH, RS_COLOR, ...
    std::vector<rsimpl::uvc::device_handle>         subdevices;                 // Indexed by UVC subdevices number (0, 1, 2...)

    bool                                            capturing;
  
public:
                                                    rs_device(rsimpl::uvc::device device, const rsimpl::static_device_info & device_info);
                                                    ~rs_device();

    const char *                                    get_name() const { return device_info.name.c_str(); }
    bool                                            supports_option(rs_option option) const { return device_info.option_supported[option]; }
    rs_extrinsics                                   get_stream_extrinsics(rs_stream from, rs_stream to) const;
    float                                           get_depth_scale() const { return device_info.depth_scale; }

    void                                            enable_stream(rs_stream stream, int width, int height, rs_format format, int fps);
    void                                            enable_stream_preset(rs_stream stream, rs_preset preset);    
    bool                                            is_stream_enabled(rs_stream stream) const { return requests[stream].enabled; }
    rsimpl::stream_mode                             get_stream_mode(rs_stream stream) const;
    rs_intrinsics                                   get_stream_intrinsics(rs_stream stream) const { return device_info.intrinsics[get_stream_mode(stream).intrinsics_index]; }
    rs_format                                       get_stream_format(rs_stream stream) const { return get_stream_mode(stream).format; }
    int                                             get_stream_framerate(rs_stream stream) const { return get_stream_mode(stream).fps; }

    void                                            start_capture();
    void                                            stop_capture();
    bool                                            is_capturing() const { return capturing; }
    
    void                                            wait_all_streams();
    int                                             get_image_frame_number(rs_stream stream) const { if(!streams[stream]) throw std::runtime_error("stream not enabled"); return streams[stream]->get_frame_number(); }
    const void *                                    get_image_pixels(rs_stream stream) const { if(!streams[stream]) throw std::runtime_error("stream not enabled"); return streams[stream]->get_image(); }
    
    void                                            configure_enabled_streams();    

    virtual void                                    set_stream_intent() = 0;
    virtual void                                    set_option(rs_option option, int value) = 0;
    virtual int                                     get_option(rs_option option) const = 0;
};

#endif
