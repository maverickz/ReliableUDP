tcpds = `pgrep tcpds`
ftps = `pgrep ftps`

kill -9 $tcpds
kill -9 $ftps
