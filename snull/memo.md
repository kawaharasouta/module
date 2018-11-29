**memo**

今の所(実装して)ない関数
 - snull_config
 - snull_poll
 - snull__napi_interrupt
 - snull_tx_timeout
 - snull__ioctl
 - snull_rebuild_header 
 - snull_change_mtu

わかんないからコメントしてるやつら
 - PDEBUGとPDEBUGGとかいうのが見つからんと言われる.
 - L351らへん, dev->trans_start がいないと言われる. netdevide.hにはちゃんといるはずなんだけどなあ. /usr/include/linux/netdevice.h見に行ったら100行も書いてなくて???ってなった. --->>> dev->openとかもいないって言われるから, これはnet_dev_optからじゃないとちゃんとならないやつなのかもしれない. でもopsは関数群な訳だからtrans_startは違うよなーんー...
 - dev->hard_headerとかhard_header_cacheとかは対応するものがなさげ.
 - NETIF_F_NO_CSUMがなかった. netdev_features.hにIP_CSUMとか他色々はあった. 

なんかのメモ
 - snull_headerとかsnull_rebuild_headerとかはheader_opsの方に書いてるやつもあった. header_opsの中はcreateとかあった.
