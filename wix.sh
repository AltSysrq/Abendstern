#! /bin/sh
autowix Abendstern.awxs && \
    candle Abendstern.wxs && \
    light -ext WixUIExtension Abendstern.wixobj
