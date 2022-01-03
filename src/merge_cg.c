#include "fuckaokana_config.h"
#include "merge_cg.h"

#include <errno.h>
#include <stdio.h>

#include "c_linked_list.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libswscale/swscale.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void free_input_avformat_context(void* p) {
    AVFormatContext* c = (AVFormatContext*)p;
    if (c) avformat_close_input(&c);
}

void free_input_avio_context(void* p) {
    AVIOContext* c = (AVIOContext*)p;
    if (c) {
        av_freep(&c->buffer);
        avio_context_free(&c);
    }
}

void free_input_codec_context(void* p) {
    AVCodecContext* c = (AVCodecContext*)p;
    avcodec_free_context(&c);
}

void free_input_filter_context(void* p) {
    AVFilterContext* c = (AVFilterContext*)p;
    avfilter_free(c);
}

int open_input(aokana_file* f, c_linked_list** ics, c_linked_list** iiocs, int verbose, int index) {
    if (!f || !ics || !iiocs) return 1;
    unsigned char* buff = NULL;
    AVIOContext* iioc = NULL;
    AVFormatContext* ic = NULL;
    /// Input file name
    char* ifn = c_get_aokana_file_full_path(f);
    int fferr = 0;
    if (!ifn) {
        printf("Out of memory.\n");
        goto end;
    }
    if (!(ic = avformat_alloc_context())) {
        printf("Out of memory.\n");
        goto end;
    }
    buff = av_malloc(4096);
    if (!buff) {
        printf("Out of memory.\n");
        goto end;
    }
    if (!(iioc = avio_alloc_context(buff, 4096, 0, (void*)f, &aokana_file_readpacket, NULL, NULL))) {
        printf("Out of memory.\n");
        goto end;
    }
    ic->pb = iioc;
    if ((fferr = avformat_open_input(&ic, ifn, NULL, NULL)) < 0) {
        printf("Failed to open input \"%s\".\n", ifn);
        goto end;
    }
    if ((fferr = avformat_find_stream_info(ic, NULL)) < 0) {
        printf("Failed to find stream info in input \"%s\".\n", ifn);
        goto end;
    }
    if (!c_linked_list_append(iiocs, (void*)iioc)) {
        printf("Out of memory.\n");
        goto end;
    }
    if (!c_linked_list_append(ics, (void*)ic)) {
        printf("Out of memory.\n");
        c_linked_list_free_tail(iiocs, &free_input_avio_context);
        goto end;
    }
    if (verbose) {
        fflush(stdout);
        av_dump_format(ic, index, ifn, 0);
    }
    fflush(stdout);
    if (ifn) free(ifn);
    return 0;
end:
    if (fferr < 0) {
        printf("FFMPEG returned %i: \"%s\".\n", fferr, av_err2str(fferr));
    }
    if (ifn) free(ifn);
    if (ic) avformat_close_input(&ic);
    if (!iioc) {
        if (buff) av_freep(&buff);
    } else {
        if (iioc->buffer) av_freep(iioc->buffer);
        avio_context_free(&iioc);
    }
    fflush(stdout);
    return 1;
}

int open_all_inputs(aokana_file_list* input, c_linked_list** ics, c_linked_list** iiocs, int verbose) {
    if (!input || !ics || !iiocs) return 1;
    aokana_file_list* t = input;
    int index = 0;
    if (t->d) {
        if (open_input(t->d, ics, iiocs, verbose, index)) return 1;
        index++;
    }
    while (t->next) {
        t = t->next;
        if (t->d) {
            if (open_input(t->d, ics, iiocs, verbose, index)) return 1;
            index++;
        }
    }
    return 0;
}

int open_input_codec(aokana_file* f, AVFormatContext* ic, c_linked_list** iccs, c_linked_list** iss, int verbose) {
    if (!f || !ic || !iccs || !iss) return 1;
    const AVCodec* codec = NULL;
    AVCodecContext* icc = NULL;
    AVStream* is = NULL;
    /// Input file name
    char* ifn = c_get_aokana_file_full_path(f);
    int fferr = 0;
    if (!ifn) {
        printf("Out of memory.\n");
        goto end;
    }
    for (unsigned int i = 0; i < ic->nb_streams; i++) {
        AVStream* s = ic->streams[i];
        if (s->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            is = s;
            break;
        }
    }
    if (!is) {
        printf("Can not find video stream in file \"%s\".\n", ifn);
        goto end;
    }
    if (!(codec = avcodec_find_decoder(is->codecpar->codec_id))) {
        printf("Can not find a suitable decoder for stream %i in file \"%s\".\n", is->index, ifn);
        goto end;
    }
    if (!(icc = avcodec_alloc_context3(codec))) {
        printf("Out of memory.\n");
        goto end;
    }
    if ((fferr = avcodec_parameters_to_context(icc, is->codecpar)) < 0) {
        printf("Failed to copy codec paramters from stream context. (Stream %i in file \"%s\").\n", is->index, ifn);
        goto end;
    }
    if ((fferr = avcodec_open2(icc, codec, NULL)) < 0) {
        printf("Failed to open decoder \"%s\". (Stream %i in file \"%s\").\n", codec->name, is->index, ifn);
        goto end;
    }
    if (!c_linked_list_append(iccs, (void*)icc)) {
        printf("Out of memory.\n");
        goto end;
    }
    if (!c_linked_list_append(iss, (void*)is)) {
        printf("Out of memory.\n");
        c_linked_list_free_tail(iccs, NULL);
        goto end;
    }
    fflush(stdout);
    if (ifn) free(ifn);
    return 0;
end:
    if (icc) avcodec_free_context(&icc);
    if (fferr < 0) {
        printf("FFMPEG returned %i: \"%s\".\n", fferr, av_err2str(fferr));
    }
    fflush(stdout);
    if (ifn) free(ifn);
    return 1;
}

int open_all_input_codecs(aokana_file_list* input, c_linked_list* ics, c_linked_list** iccs, c_linked_list** iss, int verbose) {
    if (!input || !ics || !iccs | !iss) return 1;
    aokana_file_list* f = input;
    c_linked_list* ic = ics;
    if (f->d && ic->d) {
        if (open_input_codec(f->d, (AVFormatContext*)ic->d, iccs, iss, verbose)) return 1;
    }
    while (f->next && ic->next) {
        f = f->next;
        ic = ic->next;
        if (f->d && ic->d) {
            if (open_input_codec(f->d, (AVFormatContext*)ic->d, iccs, iss, verbose)) return 1;
        }
    }
    return 0;
}

int open_output_context(AVFormatContext** oc, AVCodecContext* icc, AVCodecContext** occ, const char* output, int verbose) {
    const AVCodec* codec = NULL;
    int fferr = 0;
    AVCodecContext* tocc = NULL;
    AVStream* os = NULL;
    AVDictionary* d = NULL;
    codec = avcodec_find_encoder_by_name("libwebp");
    if (!codec) {
        printf("Can not find libwebp encoder.\n");
        goto end;
    }
    if ((fferr = avformat_alloc_output_context2(oc, NULL, "webp", output)) < 0) {
        printf("Failed to allocate output context for file \"%s\".\n", output);
        goto end;
    }
    if (!(*occ = avcodec_alloc_context3(codec))) {
        printf("Out of memory.\n");
        goto end;
    }
    tocc = *occ;
    tocc->width = icc->width;
    tocc->height = icc->height;
    tocc->sample_aspect_ratio = icc->sample_aspect_ratio;
    tocc->sample_rate = icc->sample_rate;
    tocc->pix_fmt = AV_PIX_FMT_RGB32;
    tocc->time_base = AV_TIME_BASE_Q;
    if (!(os = avformat_new_stream(*oc, codec))) {
        printf("Out of memory.\n");
        goto end;
    }
    os->time_base = AV_TIME_BASE_Q;
    av_dict_set_int(&d, "lossless", 1, 0);
    av_dict_set_int(&d, "quality", 100, 0);
    if ((fferr = avcodec_open2(tocc, codec, &d)) < 0) {
        printf("Can not open encoder \"%s\" for file \"%s\".\n", codec->name, output);
        goto end;
    }
    if ((fferr = avcodec_parameters_from_context(os->codecpar, tocc)) < 0) {
        printf("Can not copy codec paramters to stream for file \"%s\".\n", output);
        goto end;
    }
    if (verbose) {
        av_dump_format(*oc, 0, output, 1);
    }
    if (!((*oc)->oformat->flags & AVFMT_NOFILE)) {
        fferr = avio_open(&(*oc)->pb, output, AVIO_FLAG_WRITE);
        if (fferr < 0) {
            printf("Failed to open output file \"%s\".\n", output);
            goto end;
        }
    }
    if ((fferr = avformat_write_header(*oc, NULL)) < 0) {
        printf("Can not write header for file \"%s\".\n", output);
        goto end;
    }
    fflush(stdout);
    return 0;
end:
    if (fferr < 0) {
        printf("FFMPEG returned %i: \"%s\".\n", fferr, av_err2str(fferr));
    }
    fflush(stdout);
    return 1;
}

int init_filter(c_linked_list** ifcs, c_linked_list** iovfcs, AVFilterGraph* graph, AVCodecContext* icc, int index) {
    if (!ifcs || !iovfcs || !graph || !icc) return 1;
    char args[512];
    char name[64];
    const AVFilter* buffer = avfilter_get_by_name("buffer"), * overlay = NULL;
    AVFilterContext* buf = NULL, * ov = NULL;
    int fferr = 0;
    if (!buffer) {
        printf("Can not find filter buffer.\n");
        goto end;
    }
    snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", icc->width, icc->height, icc->pix_fmt, 1, AV_TIME_BASE, icc->sample_aspect_ratio.num, icc->sample_aspect_ratio.den);
    snprintf(name, 64, "in%i", index);
    if ((fferr = avfilter_graph_create_filter(&buf, buffer, name, args, NULL, graph)) < 0) {
        printf("Failed to create filter buffer.\n");
        goto end;
    }
    if (!c_linked_list_append(ifcs, (void*)buf)) {
        printf("Out of memory.\n");
        goto end;
    }
    if (!index) return 0;
    overlay = avfilter_get_by_name("overlay");
    if (!overlay) {
        printf("Can not find filter overlay.\n");
        c_linked_list_free_tail(ifcs, NULL);
        goto end;
    }
    snprintf(name, 64, "ov%i", index - 1);
    if ((fferr = avfilter_graph_create_filter(&ov, overlay, name, "format=auto", NULL, graph)) < 0) {
        printf("Failed to create filter overlay.\n");
        c_linked_list_free_tail(ifcs, NULL);
        goto end;
    }
    if (!c_linked_list_append(iovfcs, (void*)ov)) {
        printf("Out of memory.\n");
        c_linked_list_free_tail(ifcs, NULL);
        goto end;
    }
    if (index == 1) {
        AVFilterContext* ori = (AVFilterContext*)(*ifcs)->d;
        if ((fferr = avfilter_link(ori, 0, ov, 0)) < 0) {
            printf("Failed to link %s %i with %s %i.\n", ori->name, 0, ov->name, 0);
            goto end2;
        }
    } else {
        AVFilterContext* ori = (AVFilterContext*)(c_linked_list_tail(*iovfcs))->prev->d;
        if ((fferr = avfilter_link(ori, 0, ov, 0)) < 0) {
            printf("Failed to link %s %i with %s %i.\n", ori->name, 0, ov->name, 0);
            goto end2;
        }
    }
    if ((fferr = avfilter_link(buf, 0, ov, 1)) < 0) {
        printf("Failed to link %s %i with %s %i.\n", buf->name, 0, ov->name, 0);
        goto end2;
    }
    fflush(stdout);
    return 0;
end2:
    c_linked_list_free_tail(ifcs, NULL);
    c_linked_list_free_tail(iovfcs, NULL);
end:
    if (fferr < 0) {
        printf("FFMPEG returned %i: \"%s\".\n", fferr, av_err2str(fferr));
    }
    fflush(stdout);
    if (buf) avfilter_free(buf);
    if (ov) avfilter_free(ov);
    return 1;
}

int init_filters(c_linked_list** ifcs, c_linked_list** iovfcs, AVFilterGraph** graph, AVFilterContext** ofc, c_linked_list* iccs) {
    if (!ifcs || !iovfcs || !graph || !ofc || !iccs) return 1;
    int fferr = 0;
    int index = 0;
    c_linked_list* icc = iccs;
    const AVFilter* buffersink = avfilter_get_by_name("buffersink");
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_RGB32, AV_PIX_FMT_NONE };
    if (!buffersink) {
        printf("Can not find filter buffersink.\n");
        goto end;
    }
    if (!(*graph = avfilter_graph_alloc())) {
        printf("Out of memory.\n");
        goto end;
    }
    if (init_filter(ifcs, iovfcs, *graph, (AVCodecContext*)icc->d, index)) {
        goto end;
    }
    index++;
    while (icc->next) {
        icc = icc->next;
        if (init_filter(ifcs, iovfcs, *graph, (AVCodecContext*)icc->d, index)) {
            goto end;
        }
        index++;
    }
    if ((fferr = avfilter_graph_create_filter(ofc, buffersink, "out", NULL, NULL, *graph)) < 0) {
        printf("Failed to create filter buffersink.\n");
        goto end;
    }
    if ((fferr = av_opt_set_int_list(*ofc, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0) {
        printf("Failed to set flags for filter buffersink.\n");
        goto end;
    }
    if (!index) {
        AVFilterContext* ori = (AVFilterContext*)(*ifcs)->d;
        if ((fferr = avfilter_link(ori, 0, *ofc, 0)) < 0) {
            printf("Failed to link %s %i with %s %i.\n", ori->name, 0, (*ofc)->name, 0);
            goto end;
        }
    } else {
        c_linked_list* t = c_linked_list_tail(*iovfcs);
        if (!t) {
            goto end;
        }
        AVFilterContext* ori = (AVFilterContext*)t->d;
        if ((fferr = avfilter_link(ori, 0, *ofc, 0)) < 0) {
            printf("Failed to link %s %i with %s %i.\n", ori->name, 0, (*ofc)->name, 0);
            goto end;
        }
    }
    if ((fferr = avfilter_graph_config(*graph, NULL)) < 0) {
        printf("Failed to initialze filter.\n");
        goto end;
    }
    fflush(stdout);
    return 0;
end:
    if (fferr < 0) {
        printf("FFMPEG returned %i: \"%s\".\n", fferr, av_err2str(fferr));
    }
    fflush(stdout);
    return 1;
}

int decode_input(aokana_file* f, AVCodecContext* icc, AVFilterContext* ifc, AVStream* is, AVFormatContext* ic) {
    if (!f || !icc || !ifc || !is || !ic) return 1;
    int fferr = 0;
    /// Input file name
    char* ifn = c_get_aokana_file_full_path(f);
    AVPacket* pkt = NULL;
    AVFrame* frame = NULL;
    if (!ifn) {
        printf("Out of memory.\n");
        goto end;
    }
    pkt = av_packet_alloc();
    if (!pkt) {
        printf("Out of memory.\n");
        goto end;
    }
    frame = av_frame_alloc();
    if (!frame) {
        printf("Out of memory.\n");
        goto end;
    }
    while (1) {
        if ((fferr = av_read_frame(ic, pkt)) < 0) {
            printf("An error occured when reading file \"%s\".\n", ifn);
            goto end;
        }
        if (pkt->stream_index != is->index) {
            av_packet_unref(pkt);
            continue;
        }
        if ((fferr = avcodec_send_packet(icc, pkt)) < 0) {
            printf("Can not decode stream %i of file \"%s\".\n", is->index, ifn);
            av_packet_unref(pkt);
            goto end;
        }
        av_packet_unref(pkt);
        if ((fferr = avcodec_receive_frame(icc, frame)) < 0) {
            if (fferr == AVERROR(EAGAIN)) {
                fferr = 0;
                continue;
            }
            printf("Can not recive data from decoder at stream %i of file \"%s\".\n", is->index, ifn);
            goto end;
        }
        frame->pts = 0;
        if ((fferr = av_buffersrc_add_frame(ifc, frame)) < 0) {
            printf("Error when feeding the filter graph (buffer %s) at stream %i of file \"%s\".\n", ifc->name, is->index, ifn);
            goto end;
        }
        break;
    }
    if (ifn) free(ifn);
    if (pkt) av_packet_free(&pkt);
    if (frame) av_frame_free(&frame);
    fflush(stdout);
    return 0;
end:
    if (fferr < 0) {
        printf("FFMPEG returned %i: \"%s\".\n", fferr, av_err2str(fferr));
    }
    fflush(stdout);
    if (ifn) free(ifn);
    if (pkt) av_packet_free(&pkt);
    if (frame) av_frame_free(&frame);
    return 1;
}

int decode_all_inputs(aokana_file_list* input, c_linked_list* iccs, c_linked_list* ifcs, c_linked_list* iss, c_linked_list* ics) {
    if (!input || !iccs || !ifcs || !iss || !ics) return 1;
    aokana_file_list* f = input;
    c_linked_list* icc = iccs, * ifc = ifcs, * is = iss, * ic = ics;
    if (decode_input(f->d, (AVCodecContext*)icc->d, (AVFilterContext*)ifc->d, (AVStream*)is->d, (AVFormatContext*)ic->d)) {
        return 1;
    }
    while (f->next && icc->next && ifc->next && iss->next && ic->next) {
        f = f->next;
        icc = icc->next;
        ifc = ifc->next;
        is = is->next;
        ic = ic->next;
        if (decode_input(f->d, (AVCodecContext*)icc->d, (AVFilterContext*)ifc->d, (AVStream*)is->d, (AVFormatContext*)ic->d)) {
            return 1;
        }
    }
    return 0;
}

int encode_output2(AVFormatContext* oc, AVCodecContext* occ, AVFrame* frame, char* writed_data) {
    if (!oc || !occ || !writed_data) return 1;
    int fferr = 0;
    AVPacket* pkt = av_packet_alloc();
    *writed_data = 0;
    if (!pkt) {
        printf("Out of memory.\n");
        goto end;
    }
    if (frame) {
        frame->pts = 0;
        frame->pkt_dts = 0;
    }
    if ((fferr = avcodec_send_frame(occ, frame)) < 0) {
        if (fferr == AVERROR_EOF) {
            fferr = 0;
        } else {
            printf("An error occured when sending data to encoder.\n");
            goto end;
        }
    }
    if ((fferr = avcodec_receive_packet(occ, pkt)) < 0) {
        if (fferr == AVERROR_EOF || fferr == AVERROR(EAGAIN)) {
            fferr = 0;
            goto end2;
        } else {
            printf("An error occured when receving data from encoder.\n");
            goto end;
        }
    } else {
        *writed_data = 1;
    }
    if (*writed_data && pkt) {
        pkt->stream_index = 0;
        if ((fferr = av_write_frame(oc, pkt)) < 0) {
            printf("An error occured when writing data to muxer.\n");
            goto end;
        }
    }
end2:
    fflush(stdout);
    if (pkt) av_packet_free(&pkt);
    return 0;
end:
    if (fferr < 0) {
        printf("FFMPEG returned %i: \"%s\".\n", fferr, av_err2str(fferr));
    }
    fflush(stdout);
    if (pkt) av_packet_free(&pkt);
    return 1;
}

int encode_output(AVFormatContext* oc, AVCodecContext* occ, AVFilterContext* ofc) {
    if (!oc || !occ || !ofc) return 1;
    int fferr = 0;
    AVFrame* f = av_frame_alloc();
    char writed = 0;
    if (!f) {
        printf("Out of memory.\n");
        goto end;
    }
    if ((fferr = av_buffersink_get_frame(ofc, f)) < 0) {
        printf("Failed to recive data from filter \"%s\".\n", ofc->name);
        goto end;
    }
    if (encode_output2(oc, occ, f, &writed)) {
        goto end;
    }
    while (1) {
        if (encode_output2(oc, occ, NULL, &writed)) {
            goto end;
        }
        if (!writed) {
            break;
        }
    }
    if ((fferr = av_write_trailer(oc)) < 0) {
        printf("Failed write trailer.\n");
        goto end;
    }
    fflush(stdout);
    if (f) av_frame_free(&f);
    return 0;
end:
    if (fferr < 0) {
        printf("FFMPEG returned %i: \"%s\".\n", fferr, av_err2str(fferr));
    }
    fflush(stdout);
    if (f) av_frame_free(&f);
    return 1;
}

int check_input(aokana_file* f, AVStream* is, int* width, int* height) {
    if (!f || !is || !width || !height) return 1;
    int re = 0;
    /// Input file name
    char* ifn = c_get_aokana_file_full_path(f);
    if (!ifn) {
        printf("Out of memory.\n");
        goto end;
    }
    if (is->codecpar->format != AV_PIX_FMT_RGBA && is->codecpar->format != AV_PIX_FMT_ARGB) {
        printf("Warning: pixel format %s may cause loss in process.\n", av_get_pix_fmt_name(is->codecpar->format));
    }
    if (*width == 0 && *height == 0) {
        *width = is->codecpar->width;
        *height = is->codecpar->height;
    } else {
        if (*width != is->codecpar->width || *height != is->codecpar->height) {
            re = 1;
            goto end;
        }
    }
end:
    fflush(stdout);
    if (ifn) free(ifn);
    return re;
}

int check_inputs(aokana_file_list* input, c_linked_list* iss) {
    if (!input || !iss) return 1;
    int width = 0;
    int height = 0;
    aokana_file_list* f = input;
    c_linked_list* is = iss;
    if (check_input(f->d, (AVStream*)is->d, &width, &height)) {
        return 1;
    }
    while (f->next && is->next) {
        f = f->next;
        is = is->next;
        if (check_input(f->d, (AVStream*)is->d, &width, &height)) {
            return 1;
        }
    }
    return 0;
}

int merge_cgs(aokana_file_list* input, const char* output, AVDictionary* options, int verbose) {
    if (!input) return 1;
    if (verbose) av_log_set_level(AV_LOG_VERBOSE);
    /// List of input AVFormatContext
    c_linked_list* ics = NULL;
    /// List of input AVIOContext
    c_linked_list* iiocs = NULL;
    /// List of input AVCodecContext
    c_linked_list* iccs = NULL;
    /// List of input AVFilterContext for buffer
    c_linked_list* ifcs = NULL;
    /// List of input AVFilterContext for overlay
    c_linked_list* iovfcs = NULL;
    /// List of input AVStream
    c_linked_list* iss = NULL;
    AVFormatContext* oc = NULL;
    AVCodecContext* occ = NULL;
    AVFilterGraph* graph = NULL;
    AVFilterContext* ofc = NULL;
    int re = 0;
    if (open_all_inputs(input, &ics, &iiocs, verbose)) {
        re = 1;
        goto end;
    }
    if (open_all_input_codecs(input, ics, &iccs, &iss, verbose)) {
        re = 1;
        goto end;
    }
    if (!iccs->d) {
        re = 1;
        goto end;
    }
    if (check_inputs(input, iss)) {
        printf("The input file's size are not same.\n");
        fflush(stdout);
        re = 1;
        goto end;
    }
    if (open_output_context(&oc, (AVCodecContext*)iccs->d, &occ, output, verbose)) {
        re = 1;
        goto end;
    }
    if (init_filters(&ifcs, &iovfcs, &graph, &ofc, iccs)) {
        re = 1;
        goto end;
    }
    if (decode_all_inputs(input, iccs, ifcs, iss, ics)) {
        re = 1;
        goto end;
    }
    if (encode_output(oc, occ, ofc)) {
        re = 1;
        goto end;
    }
end:
    if (graph) avfilter_graph_free(&graph);
    if (occ) avcodec_free_context(&occ);
    if (oc) {
        if (!(oc->oformat->flags & AVFMT_NOFILE)) avio_closep(&oc->pb);
        avformat_free_context(oc);
    }
    c_linked_list_clear(&ifcs, NULL);
    c_linked_list_clear(&iovfcs, NULL);
    c_linked_list_clear(&iccs, &free_input_codec_context);
    c_linked_list_clear(&ics, &free_input_avformat_context);
    c_linked_list_clear(&iiocs, &free_input_avio_context);
    c_linked_list_clear(&iss, NULL);
    return re;
}
