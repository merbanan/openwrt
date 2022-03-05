--- a/include/uapi/linux/nl80211.h
+++ b/include/uapi/linux/nl80211.h
@@ -2615,6 +2615,10 @@ enum nl80211_commands {
  *	switching on a different channel during CAC detection on the selected
  *	radar channel.
  *
+ * @NL80211_ATTR_AP_SETTINGS_FLAGS: u32 attribute contains ap settings flags,
+ *	enumerated in &enum nl80211_ap_settings_flags. This attribute shall be
+ *	used with %NL80211_CMD_START_AP request.
+ *
  * @NL80211_ATTR_WIPHY_ANTENNA_GAIN: Configured antenna gain. Used to reduce
  *	transmit power to stay within regulatory limits. u32, dBi.
  *
@@ -3126,6 +3130,8 @@ enum nl80211_attrs {
 
 	NL80211_ATTR_RADAR_BACKGROUND,
 
+	NL80211_ATTR_AP_SETTINGS_FLAGS,
+
 	NL80211_ATTR_WIPHY_ANTENNA_GAIN,
 
 	/* add attributes here, update the policy in nl80211.c */
@@ -6027,6 +6033,11 @@ enum nl80211_feature_flags {
  * @NL80211_EXT_FEATURE_BSS_COLOR: The driver supports BSS color collision
  *	detection and change announcemnts.
  *
+ * @NL80211_EXT_FEATURE_FILS_CRYPTO_OFFLOAD: Driver running in AP mode supports
+ *	FILS encryption and decryption for (Re)Association Request and Response
+ *	frames. Userspace has to share FILS AAD details to the driver by using
+ *	@NL80211_CMD_SET_FILS_AAD.
+ *
  * @NL80211_EXT_FEATURE_RADAR_BACKGROUND: Device supports background radar/CAC
  *	detection.
  *
@@ -6095,6 +6106,7 @@ enum nl80211_ext_feature_index {
 	NL80211_EXT_FEATURE_SECURE_RTT,
 	NL80211_EXT_FEATURE_PROT_RANGE_NEGO_AND_MEASURE,
 	NL80211_EXT_FEATURE_BSS_COLOR,
+	NL80211_EXT_FEATURE_FILS_CRYPTO_OFFLOAD,
 	NL80211_EXT_FEATURE_RADAR_BACKGROUND,
 
 	/* add new features before the definition below */
