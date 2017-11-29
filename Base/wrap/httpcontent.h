﻿#ifndef HEADER_CURL_CONTENT_ENCODING_H
#define HEADER_CURL_CONTENT_ENCODING_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2011, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/*
 * Comma-separated list all supported Content-Encodings ('identity' is implied)
 */
#define ALL_CONTENT_ENCODINGS "deflate, gzip"

namespace Wrap{
	class CHttpDownload;
}
/* force a cleanup */
void Http_unencode_cleanup(Wrap::CHttpDownload* k);

int Http_unencode_deflate_write(Wrap::CHttpDownload* k, unsigned char* httbBuf, unsigned int httpBufLen);

int Http_unencode_gzip_write(Wrap::CHttpDownload* k, unsigned char* httbBuf, unsigned int httpBufLen);

#endif /* HEADER_CURL_CONTENT_ENCODING_H */
