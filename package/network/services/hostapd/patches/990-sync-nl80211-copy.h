--- a/src/drivers/nl80211_copy.h
+++ b/src/drivers/nl80211_copy.h
@@ -11,7 +11,7 @@
  * Copyright 2008 Jouni Malinen <jouni.malinen@atheros.com>
  * Copyright 2008 Colin McCabe <colin@cozybit.com>
  * Copyright 2015-2017	Intel Deutschland GmbH
- * Copyright (C) 2018-2020 Intel Corporation
+ * Copyright (C) 2018-2021 Intel Corporation
  *
  * Permission to use, copy, modify, and/or distribute this software for any
  * purpose with or without fee is hereby granted, provided that the above
@@ -337,7 +337,10 @@
  * @NL80211_CMD_DEL_INTERFACE: Virtual interface was deleted, has attributes
  *	%NL80211_ATTR_IFINDEX and %NL80211_ATTR_WIPHY. Can also be sent from
  *	userspace to request deletion of a virtual interface, then requires
- *	attribute %NL80211_ATTR_IFINDEX.
+ *	attribute %NL80211_ATTR_IFINDEX. If multiple BSSID advertisements are
+ *	enabled using %NL80211_ATTR_MBSSID_CONFIG, %NL80211_ATTR_MBSSID_ELEMS,
+ *	and if this command is used for the transmitting interface, then all
+ *	the non-transmitting interfaces are deleted as well.
  *
  * @NL80211_CMD_GET_KEY: Get sequence counter information for a key specified
  *	by %NL80211_ATTR_KEY_IDX and/or %NL80211_ATTR_MAC.
@@ -1185,6 +1188,21 @@
  *	passed using %NL80211_ATTR_SAR_SPEC. %NL80211_ATTR_WIPHY is used to
  *	specify the wiphy index to be applied to.
  *
+ * @NL80211_CMD_OBSS_COLOR_COLLISION: This notification is sent out whenever
+ *	mac80211/drv detects a bss color collision.
+ *
+ * @NL80211_CMD_COLOR_CHANGE_REQUEST: This command is used to indicate that
+ *	userspace wants to change the BSS color.
+ *
+ * @NL80211_CMD_COLOR_CHANGE_STARTED: Notify userland, that a color change has
+ *	started
+ *
+ * @NL80211_CMD_COLOR_CHANGE_ABORTED: Notify userland, that the color change has
+ *	been aborted
+ *
+ * @NL80211_CMD_COLOR_CHANGE_COMPLETED: Notify userland that the color change
+ *	has completed
+ *
  * @NL80211_CMD_MAX: highest used command number
  * @__NL80211_CMD_AFTER_LAST: internal use
  */
@@ -1417,6 +1435,14 @@ enum nl80211_commands {
 
 	NL80211_CMD_SET_SAR_SPECS,
 
+	NL80211_CMD_OBSS_COLOR_COLLISION,
+
+	NL80211_CMD_COLOR_CHANGE_REQUEST,
+
+	NL80211_CMD_COLOR_CHANGE_STARTED,
+	NL80211_CMD_COLOR_CHANGE_ABORTED,
+	NL80211_CMD_COLOR_CHANGE_COMPLETED,
+
 	/* add new commands above here */
 
 	/* used to define NL80211_CMD_MAX below */
@@ -2560,6 +2586,39 @@ enum nl80211_commands {
  *	disassoc events to indicate that an immediate reconnect to the AP
  *	is desired.
  *
+ * @NL80211_ATTR_OBSS_COLOR_BITMAP: bitmap of the u64 BSS colors for the
+ *	%NL80211_CMD_OBSS_COLOR_COLLISION event.
+ *
+ * @NL80211_ATTR_COLOR_CHANGE_COUNT: u8 attribute specifying the number of TBTT's
+ *	until the color switch event.
+ * @NL80211_ATTR_COLOR_CHANGE_COLOR: u8 attribute specifying the color that we are
+ *	switching to
+ * @NL80211_ATTR_COLOR_CHANGE_ELEMS: Nested set of attributes containing the IE
+ *	information for the time while performing a color switch.
+ *
+ * @NL80211_ATTR_MBSSID_CONFIG: Nested attribute for multiple BSSID
+ *	advertisements (MBSSID) parameters in AP mode.
+ *	Kernel uses this attribute to indicate the driver's support for MBSSID
+ *	and enhanced multi-BSSID advertisements (EMA AP) to the userspace.
+ *	Userspace should use this attribute to configure per interface MBSSID
+ *	parameters.
+ *	See &enum nl80211_mbssid_config_attributes for details.
+ *
+ * @NL80211_ATTR_MBSSID_ELEMS: Nested parameter to pass multiple BSSID elements.
+ *	Mandatory parameter for the transmitting interface to enable MBSSID.
+ *	Optional for the non-transmitting interfaces.
+ *
+ * @NL80211_ATTR_RADAR_BACKGROUND: Configure dedicated offchannel chain
+ *	available for radar/CAC detection on some hw. This chain can't be used
+ *	to transmit or receive frames and it is bounded to a running wdev.
+ *	Background radar/CAC detection allows to avoid the CAC downtime
+ *	switching on a different channel during CAC detection on the selected
+ *	radar channel.
+ *
+ * @NL80211_ATTR_AP_SETTINGS_FLAGS: u32 attribute contains ap settings flags,
+ *	enumerated in &enum nl80211_ap_settings_flags. This attribute shall be
+ *	used with %NL80211_CMD_START_AP request.
+ *
  * @NUM_NL80211_ATTR: total number of nl80211_attrs available
  * @NL80211_ATTR_MAX: highest attribute number currently defined
  * @__NL80211_ATTR_AFTER_LAST: internal use
@@ -3057,6 +3116,19 @@ enum nl80211_attrs {
 
 	NL80211_ATTR_DISABLE_HE,
 
+	NL80211_ATTR_OBSS_COLOR_BITMAP,
+
+	NL80211_ATTR_COLOR_CHANGE_COUNT,
+	NL80211_ATTR_COLOR_CHANGE_COLOR,
+	NL80211_ATTR_COLOR_CHANGE_ELEMS,
+
+	NL80211_ATTR_MBSSID_CONFIG,
+	NL80211_ATTR_MBSSID_ELEMS,
+
+	NL80211_ATTR_RADAR_BACKGROUND,
+
+	NL80211_ATTR_AP_SETTINGS_FLAGS,
+
 	/* add attributes here, update the policy in nl80211.c */
 
 	__NL80211_ATTR_AFTER_LAST,
@@ -3654,6 +3726,8 @@ enum nl80211_mpath_info {
  *     defined
  * @NL80211_BAND_IFTYPE_ATTR_HE_6GHZ_CAPA: HE 6GHz band capabilities (__le16),
  *	given for all 6 GHz band channels
+ * @NL80211_BAND_IFTYPE_ATTR_VENDOR_ELEMS: vendor element capabilities that are
+ *	advertised on this band/for this iftype (binary)
  * @__NL80211_BAND_IFTYPE_ATTR_AFTER_LAST: internal use
  */
 enum nl80211_band_iftype_attr {
@@ -3665,6 +3739,7 @@ enum nl80211_band_iftype_attr {
 	NL80211_BAND_IFTYPE_ATTR_HE_CAP_MCS_SET,
 	NL80211_BAND_IFTYPE_ATTR_HE_CAP_PPE,
 	NL80211_BAND_IFTYPE_ATTR_HE_6GHZ_CAPA,
+	NL80211_BAND_IFTYPE_ATTR_VENDOR_ELEMS,
 
 	/* keep last */
 	__NL80211_BAND_IFTYPE_ATTR_AFTER_LAST,
@@ -5950,6 +6025,17 @@ enum nl80211_feature_flags {
  *      frame protection for all management frames exchanged during the
  *      negotiation and range measurement procedure.
  *
+ * @NL80211_EXT_FEATURE_BSS_COLOR: The driver supports BSS color collision
+ *	detection and change announcemnts.
+ *
+ * @NL80211_EXT_FEATURE_FILS_CRYPTO_OFFLOAD: Driver running in AP mode supports
+ *	FILS encryption and decryption for (Re)Association Request and Response
+ *	frames. Userspace has to share FILS AAD details to the driver by using
+ *	@NL80211_CMD_SET_FILS_AAD.
+ *
+ * @NL80211_EXT_FEATURE_RADAR_BACKGROUND: Device supports background radar/CAC
+ *	detection.
+ *
  * @NUM_NL80211_EXT_FEATURES: number of extended features.
  * @MAX_NL80211_EXT_FEATURES: highest extended feature index.
  */
@@ -6014,6 +6100,9 @@ enum nl80211_ext_feature_index {
 	NL80211_EXT_FEATURE_SECURE_LTF,
 	NL80211_EXT_FEATURE_SECURE_RTT,
 	NL80211_EXT_FEATURE_PROT_RANGE_NEGO_AND_MEASURE,
+	NL80211_EXT_FEATURE_BSS_COLOR,
+	NL80211_EXT_FEATURE_FILS_CRYPTO_OFFLOAD,
+	NL80211_EXT_FEATURE_RADAR_BACKGROUND,
 
 	/* add new features before the definition below */
 	NUM_NL80211_EXT_FEATURES,
@@ -6912,6 +7001,9 @@ enum nl80211_peer_measurement_ftm_capa {
  * @NL80211_PMSR_FTM_REQ_ATTR_LMR_FEEDBACK: negotiate for LMR feedback. Only
  *	valid if either %NL80211_PMSR_FTM_REQ_ATTR_TRIGGER_BASED or
  *	%NL80211_PMSR_FTM_REQ_ATTR_NON_TRIGGER_BASED is set.
+ * @NL80211_PMSR_FTM_REQ_ATTR_BSS_COLOR: optional. The BSS color of the
+ *	responder. Only valid if %NL80211_PMSR_FTM_REQ_ATTR_NON_TRIGGER_BASED
+ *	or %NL80211_PMSR_FTM_REQ_ATTR_TRIGGER_BASED is set.
  *
  * @NUM_NL80211_PMSR_FTM_REQ_ATTR: internal
  * @NL80211_PMSR_FTM_REQ_ATTR_MAX: highest attribute number
@@ -6931,6 +7023,7 @@ enum nl80211_peer_measurement_ftm_req {
 	NL80211_PMSR_FTM_REQ_ATTR_TRIGGER_BASED,
 	NL80211_PMSR_FTM_REQ_ATTR_NON_TRIGGER_BASED,
 	NL80211_PMSR_FTM_REQ_ATTR_LMR_FEEDBACK,
+	NL80211_PMSR_FTM_REQ_ATTR_BSS_COLOR,
 
 	/* keep last */
 	NUM_NL80211_PMSR_FTM_REQ_ATTR,
@@ -7299,4 +7392,60 @@ enum nl80211_sar_specs_attrs {
 	NL80211_SAR_ATTR_SPECS_MAX = __NL80211_SAR_ATTR_SPECS_LAST - 1,
 };
 
+/**
+ * enum nl80211_mbssid_config_attributes - multiple BSSID (MBSSID) and enhanced
+ * multi-BSSID advertisements (EMA) in AP mode.
+ * Kernel uses some of these attributes to advertise driver's support for
+ * MBSSID and EMA.
+ * Remaining attributes should be used by the userspace to configure the
+ * features.
+ *
+ * @__NL80211_MBSSID_CONFIG_ATTR_INVALID: Invalid
+ *
+ * @NL80211_MBSSID_CONFIG_ATTR_MAX_INTERFACES: Used by the kernel to advertise
+ *	the maximum number of MBSSID interfaces supported by the driver.
+ *	Driver should indicate MBSSID support by setting
+ *	wiphy->mbssid_max_interfaces to a value more than or equal to 2.
+ *
+ * @NL80211_MBSSID_CONFIG_ATTR_MAX_EMA_PROFILE_PERIODICITY: Used by the kernel
+ *	to advertise the maximum profile periodicity supported by the driver
+ *	if EMA is enabled. Driver should indicate EMA support to the userspace
+ *	by setting wiphy->mbssid_max_ema_profile_periodicity to
+ *	a non-zero value.
+ *
+ * @NL80211_MBSSID_CONFIG_ATTR_INDEX: Mandatory parameter to pass the index of
+ *	this BSS (u8) in the multiple BSSID set.
+ *	Value must be set to 0 for the transmitting interface and non-zero for
+ *	all non-transmitting interfaces. The userspace will be responsible
+ *	for using unique indices for the interfaces.
+ *	Range: 0 to wiphy->mbssid_max_interfaces-1.
+ *
+ * @NL80211_MBSSID_CONFIG_ATTR_TX_IFINDEX: Mandatory parameter for
+ *	a non-transmitted profile which provides the interface index (u32) of
+ *	the transmitted profile. The value must match one of the interface
+ *	indices advertised by the kernel. Optional if the interface being set up
+ *	is the transmitting one, however, if provided then the value must match
+ *	the interface index of the same.
+ *
+ * @NL80211_MBSSID_CONFIG_ATTR_EMA: Flag used to enable EMA AP feature.
+ *	Setting this flag is permitted only if the driver advertises EMA support
+ *	by setting wiphy->mbssid_max_ema_profile_periodicity to non-zero.
+ *
+ * @__NL80211_MBSSID_CONFIG_ATTR_LAST: Internal
+ * @NL80211_MBSSID_CONFIG_ATTR_MAX: highest attribute
+ */
+enum nl80211_mbssid_config_attributes {
+	__NL80211_MBSSID_CONFIG_ATTR_INVALID,
+
+	NL80211_MBSSID_CONFIG_ATTR_MAX_INTERFACES,
+	NL80211_MBSSID_CONFIG_ATTR_MAX_EMA_PROFILE_PERIODICITY,
+	NL80211_MBSSID_CONFIG_ATTR_INDEX,
+	NL80211_MBSSID_CONFIG_ATTR_TX_IFINDEX,
+	NL80211_MBSSID_CONFIG_ATTR_EMA,
+
+	/* keep last */
+	__NL80211_MBSSID_CONFIG_ATTR_LAST,
+	NL80211_MBSSID_CONFIG_ATTR_MAX = __NL80211_MBSSID_CONFIG_ATTR_LAST - 1,
+};
+
 #endif /* __LINUX_NL80211_H */
