platform_do_upgrade() {
	local board=$(board_name)

	case "$board" in
	*)
		nand_do_upgrade "$1"
		;;
	esac
}

PART_NAME=tclinux

platform_check_image() {
	local board=$(board_name)

	case "$board" in
	*)
		nand_do_platform_check "$board" "$1"
		return $?
		;;
	esac

	return 0
}
