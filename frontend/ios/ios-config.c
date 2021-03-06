/*
 *  CVS Version: $Id: ios-config.c,v 1.8 2013/08/05 14:31:08 olof Exp $
 *
 *  Copyright (C) 2009-2014 Olof Hagsand and Benny Holmgren
 *
 *  This file is part of ROST.
 *
 *  ROST is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ROST is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along wth ROST; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#ifdef HAVE_ROST_CONFIG_H
#include "rost_config.h" /* generated by config & autoconf */
#endif /* HAVE_ROST_CONFIG_H */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

/* clicon */
#include <cligen/cligen.h>
#include <clicon/clicon.h>
#include <clicon/clicon_cli.h>

/* local */
#include "ios.h"


static struct lvmap ios_if_fmts[] = {
    {".description", " description $description", NULL, LVPRINT_CMD, NULL},
    {".dot1q", " encapsulation dot1Q $vlan", NULL, LVPRINT_CMD, NULL},
/*    {".tunnel.mode", " tunnel mode $mode", NULL, LVPRINT_CMD, NULL},*/
    {".tunnel.source", " tunnel source $source", NULL, LVPRINT_CMD, NULL},
    {".tunnel.destination", " tunnel destination $destination", NULL, LVPRINT_CMD, NULL},
    {".tunnel.tos", " tunnel tos $tos", NULL, LVPRINT_CMD, NULL},
    {".tunnel.key", " tunnel key $key", NULL, LVPRINT_CMD, NULL},
    {".tunnel.nopmtu", " no tunnel path-mtu-discovery", NULL, LVPRINT_CMD, NULL},
    {".tunnel.ttl", " tunnel ttl $ttl", NULL, LVPRINT_CMD, NULL},
    {".tunnel.csum", " tunnel checksum $csum", NULL, LVPRINT_CMD, NULL},
    {".inet.dhcp_client", " ip address dhcp", NULL, LVPRINT_CMD, NULL},
#if 1 /* ??*/
    {".inet.address[]", " ip address $prefix", NULL, LVPRINT_CMD, NULL},
#else
    {".inet.address[]", " ip address $prefix", " no ip address", LVPRINT_CMD, NULL},
#endif
#ifdef ROST_IPV6_SUPPORT
    {".ipv6.address[]", " ipv6 address $prefix", NULL, LVPRINT_CMD, NULL},
    {".ipv6.nd.adv-interval-option", " ipv6 nd adv-interval-option", NULL, LVPRINT_CMD, NULL},
#endif

    {".bandwidth", " bandwidth $bandwidth", NULL, LVPRINT_CMD, NULL},
    {".link-detect", " link-detect", NULL, LVPRINT_CMD, NULL},
    {".shutdown", " shutdown", NULL, LVPRINT_CMD, NULL},
    {".ipv4.rp_filter", "$status{$${%n{1}?%s{ ip verify unicast source reachable-via rx}:%s{}}}", NULL, LVPRINT_CMD, NULL},
    {".ipv4.send_redirects", "$status{$${%n{0}?%s{ no ip redirects}:%s{}}}", NULL, LVPRINT_CMD, NULL},
    {".ipv4.proxy_arp", "$status{$${%n{0}?%s{ no ip proxy-arp}:%s{}}}", NULL,  LVPRINT_CMD, NULL},
    {".ospf.cost", " ip ospf cost $cost", NULL, LVPRINT_CMD, NULL},
    {".ospf.network-type", " ip ospf network-type $network_type", NULL, LVPRINT_CMD, NULL},
    {".ospf.priority", " ip ospf priority $priority", NULL, LVPRINT_CMD, NULL},
    {".rip.receive", " ip rip receive version $version", NULL, LVPRINT_CMD, NULL},
    {".rip.send", " ip rip send version $version", NULL, LVPRINT_CMD, NULL},
    {".rip.split-horizon", "$no{ no ip rip split-horizon}$poisoned_reverse{ ip rip split-horizon $$}", NULL, LVPRINT_CMD, NULL},
    
    {"#", "!", "!", LVPRINT_COMMENT, NULL},

    {NULL, NULL, NULL}
};

static struct lvmap ios_rip_fmts[] = {
    {".version", " version $version", NULL, LVPRINT_CMD, NULL},
    {".timers.basic", " timers basic $update $timeout $garbage", NULL, LVPRINT_CMD, NULL},
    {".default-information", " default-information originate", NULL, LVPRINT_CMD, NULL},
    {".redistribute[]", " redistribute $protocol$metric{ metric $$}$route_map{ route-map $$}", NULL, LVPRINT_CMD, NULL},
    {".offset-list[]", " offset-list $acl $direction $metric$interface{ $$}", NULL, LVPRINT_CMD, NULL},
    {".network.prefix[]", " network $prefix", NULL, LVPRINT_CMD, NULL},
    {".network.interface[].unit[]", " network $interface", NULL, LVPRINT_CMD, NULL},
    {".neighbor[]", " neighbor $neighbor", NULL, LVPRINT_CMD, NULL},
    {".passive-interface[].unit[]", " passive-interface $interface", NULL, LVPRINT_CMD, NULL},
    {".default-metric", " default-metric $metric", NULL, LVPRINT_CMD, NULL},
    {".distribute-list[]", " distribute-list $acl $direction$interface{ $$}", NULL, LVPRINT_CMD, NULL},
    {".distribute-list.prefix[]", " distribute-list prefix $prefix $direction$interface{ $$}", NULL, LVPRINT_CMD, NULL},
    {".route-map[]", " route-map $route_map $direction $interface", NULL, LVPRINT_CMD, NULL},
    {".distance", " distance $distance", NULL, LVPRINT_CMD, NULL},
    {".distance.prefix[]", " distance $distance $prefix$acl{ $$}", NULL, LVPRINT_CMD, NULL},

};

static struct lvmap ios_ospf_fmts[] = {
    {".router-id", " router-id $routerid", NULL, LVPRINT_CMD, NULL},
    {".abr-type", " ospf abr-type $abr_type", NULL, LVPRINT_CMD, NULL},
    {".log-adjacency-changes", " log-adjacency-changes$detail{ $$}", NULL, LVPRINT_CMD, NULL},
    {".compatible", " compatible rfc1583",NULL, LVPRINT_CMD, NULL},
    {".auto-cost.reference-bandwidth", " auto-cost reference-bandwidth $bandwidth",NULL, LVPRINT_CMD, NULL},
    {".timers.throttle.spf", " timers throttle spf $delay $initial_hold $max_hold", NULL, LVPRINT_CMD, NULL},
    {".max-metric.router-lsa.on-startup", " max-metric router-lsa on-startup $on_startup", NULL, LVPRINT_CMD, NULL},
    {".max-metric.router-lsa.on-shutdown", " max-metric router-lsa on-shutdown $on_shutdown", NULL, LVPRINT_CMD, NULL},
    {".max-metric.router-lsa.administrative", " max-metric router-lsa administrative", NULL, LVPRINT_CMD, NULL},
    {".refresh-timer", " refresh timer $timer", NULL, LVPRINT_CMD, NULL},
    {".redistribute.bgp", " redistribute bgp$metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}", NULL, LVPRINT_CMD, NULL},
    {".redistribute.connected", " redistribute connected$metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}", NULL, LVPRINT_CMD, NULL},
#ifdef ROST_ISIS_SUPPORT
    {".redistribute.isis", " redistribute isis$metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}", NULL, LVPRINT_CMD, NULL},
#endif
    {".redistribute.kernel", " redistribute kernel$metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}", NULL, LVPRINT_CMD, NULL},
    {".redistribute.rip", " redistribute rip$metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}", NULL, LVPRINT_CMD, NULL},
    {".redistribute.static", " redistribute static$metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}", NULL, LVPRINT_CMD, NULL},
    {".passive-interface[]", " passive-interface $interface", NULL, LVPRINT_CMD, NULL},
    {".network[]", " network $prefix area $area", NULL, LVPRINT_CMD, NULL},
    {".area[].auth", " area $area authentication$auth{ $$}", NULL, LVPRINT_CMD, NULL},
    {".area[].shortcut", " area $area shortcut $mode", NULL, LVPRINT_CMD, NULL},
    {".area[].stub", " area $area stub$no_summary{ $$}", NULL, LVPRINT_CMD, NULL},
    {".area[].default-cost", " area $area default-cost $cost", NULL, LVPRINT_CMD, NULL},
    {".area[].export-list", " area $area export-list $export_list", NULL, LVPRINT_CMD, NULL},
    {".area[].import-list", " area $area import-list $import_list", NULL, LVPRINT_CMD, NULL},
    {".area[].nssa", " area $area nssa$translate{ translate-$$}$no_summary{ $$}", NULL, LVPRINT_CMD, NULL},
    {".area[].range[]", " area $area range $range$substitute{ substitute $$}$not_advertise{ $$}$cost{ cost $$}", NULL, LVPRINT_CMD},
    {".neighbor[]", " neighbor $neighbor$priority{ priority $$}$poll_interval{ poll-interval $$}", NULL, LVPRINT_CMD, NULL},
    {".default-metric", " default-metric $metric", NULL, LVPRINT_CMD},
    {".distribute-list[]", " distribute-list $acl $direction $protocol", NULL, LVPRINT_CMD, NULL},
    {".distance", " distance $distance", NULL, LVPRINT_CMD},
    {".default-information.originate", " default-information originate$always{ $$}$metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}", NULL, LVPRINT_CMD},

    {".capability", " capability opaque", NULL, LVPRINT_CMD, NULL},
    {"#", "!", "!", LVPRINT_COMMENT, NULL},
    {NULL, NULL, NULL}
};
    
static struct lvmap ios_bgp_fmts[] = {
  
  {".router-id", " bgp router-id $routerid", NULL, LVPRINT_CMD, NULL},
  {".log-neighbor-changes", " bgp log-neighbor-changes", NULL, LVPRINT_CMD, NULL},
  {".always-compare-med", " bgp always-compare-med", NULL, LVPRINT_CMD, NULL},
  {".default.local-preference", " bgp default local-preference $local_preference", NULL, LVPRINT_CMD, NULL},
  {".enforce-first-as", " bgp enforce-first-as", NULL, LVPRINT_CMD, NULL},
  {".deterministic-med", " bgp deterministic-med", NULL, LVPRINT_CMD, NULL},
  {".graceful-restart", " bgp graceful-restart$stalepath_time{ stalepath-time $$}", NULL, LVPRINT_CMD, NULL},
  {".network.import-check", " bgp network import-check", NULL, LVPRINT_CMD, NULL},
  {".dampening", " bgp dampening$half_life{ $$}$reuse{ $$}$suppress{ $$}$max_suppress{ $$}", NULL, LVPRINT_CMD, NULL},
  {".network[]", " network $prefix$route_map{ route-map $$}", NULL, LVPRINT_CMD, NULL},
  {".aggregate-address[]", " aggregate-address $prefix$as_set{ $$}$summary_only{ $$}", NULL, LVPRINT_CMD, NULL},
  {".redistribute.connected", " redistribute connected$metric{ metric $$}$route_map{ route-map $$}", NULL, LVPRINT_CMD, NULL},
  {".redistribute.kernel", " redistribute kernel$metric{ metric $$}$route_map{ route-map $$}", NULL, LVPRINT_CMD, NULL},
  {".redistribute.ospf", " redistribute ospf$metric{ metric $$}$route_map{ route-map $$}", NULL, LVPRINT_CMD, NULL},
  {".redistribute.rip", " redistribute rip$metric{ metric $$}$route_map{ route-map $$}", NULL, LVPRINT_CMD, NULL},
  {".redistribute.static", " redistribute static$metric{ metric $$}$route_map{ route-map $$}", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].remote-as", " neighbor $neighbor remote-as $remote_as", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].local-as", " neighbor $neighbor local-as $localas$no_prepend{ $$}", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].description", " neighbor $neighbor description $description", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].shutdown", " neighbor $neighbor shutdown", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].password", " neighbor $neighbor password $password", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].passive", " neighbor $neighbor passive", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].ebgp-multihop", " neighbor $neighbor ebgp-multihop$maxhops{ $$}", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].update-source", " neighbor $neighbor update-source $source", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].advertisement-interval", " neighbor $neighbor advertisement-interval $interval", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].timers", " neighbor $neighbor timers $keepalive $holdtime", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].timers.connect", " neighbor $neighbor timers connect $connect", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].weight", " neighbor $neighbor weight $weight", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].next-hop-self", " neighbor $neighbor next-hop-self", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].remove-private-as", " neighbor $neighbor remove-private-as", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].default-originate", " neighbor $neighbor default-originate$route_map{ route-map $$}", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].soft-reconfiguration.inbound", " neighbor $neighbor soft-reconfiguration inbound", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].prefix-list.in", " neighbor $neighbor prefix-list $prefix_list in", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].prefix-listout", " neighbor $neighbor prefix-list $prefix_list out", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].route-map.in", " neighbor $neighbor route-map $route_map in", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].route-map.out", " neighbor $neighbor route-map $route_map out", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].distribute-list.in", " neighbor $neighbor distribute-list $acl in", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].distribute-list.out", " neighbor $neighbor distribute-list $acl out", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].filter-list.in", " neighbor $neighbor filter-list $acl in", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].filter-list.in", " neighbor $neighbor filter-list $acl out", NULL, LVPRINT_CMD, NULL},
  {".neighbor[].activate", " neighbor $neighbor activate", NULL, LVPRINT_CMD, NULL},
    {".distance.bgp", " distance bgp $external $internal $local", NULL, LVPRINT_CMD, NULL},
    {".distance[]", " distance $distance $prefix$acl{ $$}", NULL, LVPRINT_CMD, NULL},

    {"#", "!", "!", LVPRINT_COMMENT, NULL},
    {NULL, NULL, NULL}
};
    
static struct lvmap ios_route_map_fmts[] = {
  
    {".call", " call $route_map", NULL, LVPRINT_CMD, NULL},
    {".description", " description $description", NULL, LVPRINT_CMD, NULL},
    {".match.as-path", " match as-path $as_path", NULL, LVPRINT_CMD, NULL},
    {".match.interface", " match interface $interface", NULL, LVPRINT_CMD, NULL},
    {".match.ip.address", " match ip address $acl", NULL, LVPRINT_CMD, NULL},
    {".match.ip.address.prefix-list", " match ip address prefix-list $prefix_list", NULL, LVPRINT_CMD, NULL},
    {".match.ip.next-hop", " match ip next-hop $acl", NULL, LVPRINT_CMD, NULL},
    {".match.ip.next-hop.prefix-list", " match ip next-hop prefix-list $prefix_list", NULL, LVPRINT_CMD, NULL},
    {".match.ip.route-source", " match ip route-source $acl", NULL, LVPRINT_CMD, NULL},
    {".match.ip.route-source.prefix-list", " match ip route-source prefix-list $prefix_list", NULL, LVPRINT_CMD, NULL},
    {".match.metric", " match metric $metric", NULL, LVPRINT_CMD, NULL},
    {".on-match", " on-match $op$goto{ $$}", NULL, LVPRINT_CMD, NULL},
    {".set.as-path.exclude", " set as-path exclude $as_path", NULL, LVPRINT_CMD, NULL},
    {".set.as-path.prepend", " set as-path prepend $as_path", NULL, LVPRINT_CMD, NULL},
    {".set.ip.next-hop", " set ip next-hop $nexthop", NULL, LVPRINT_CMD, NULL},
    {".set.local-preference", " set local-preference $localpref", NULL, LVPRINT_CMD, NULL},
    {".set.metric", " set metric$op{ $$} $metric", NULL, LVPRINT_CMD, NULL},
    {".set.metric-type", " set metric-type $metric_type", NULL, LVPRINT_CMD, NULL},
    {".set.origin", " set origin $origin", NULL, LVPRINT_CMD, NULL},
    {".set.originator-id", " set originator-id $originator_id", NULL, LVPRINT_CMD, NULL},
    {".set.pathlimit.ttl", " set pathlimit ttl $ttl", NULL, LVPRINT_CMD, NULL},
    {".set.tag", " set tag $route_tag", NULL, LVPRINT_CMD, NULL},
    {".set.weight", " set weight $weight", NULL, LVPRINT_CMD, NULL},
    {".continue", " continue $goto", NULL, LVPRINT_CMD, NULL},

    {"#", "!", "!", LVPRINT_COMMENT, NULL},
    {NULL, NULL, NULL}
};

static struct lvmap ios_user_fmts[] = {
    {".uid", "username $user uid $uid", NULL, LVPRINT_CMD}, /* SHould be first of user statement`s */
    {".authentication.password", "username $user password $password", NULL, LVPRINT_CMD},
    {".class", "username $user class $class", NULL, LVPRINT_CMD},
    {NULL, NULL, NULL}
};
    
 /* How to transform from db to IOS */
static struct lvmap ios_fmts[] = {
    {"system.hostname", "hostname $hostname", NULL, LVPRINT_CMD},
    {"#", "!", "!", LVPRINT_COMMENT},
    {"system.boot", "boot system flash $title", NULL, LVPRINT_FUNC, LVPRINT_FUNC_GRUB},
    {"#", "!", "!", LVPRINT_COMMENT},
#if nousersorting
    {"system.login.user[].uid", "username $user uid $uid", NULL, LVPRINT_CMD}, /* SHould be first of user statements */
    {"system.login.user[].authentication.password", "username $user password $password", NULL, LVPRINT_CMD},
    {"system.login.user[].class", "username $user class $class", NULL, LVPRINT_CMD},
#else
    {"system.login.user[]", "", NULL, LVPRINT_MODE, ios_user_fmts},
#endif

    {"#", "!", "!", LVPRINT_COMMENT},
    {"ipv4.domain", "ip domain name $domain", NULL, LVPRINT_CMD},
    {"ipv4.name-server[]", "ip name-server $address", NULL, LVPRINT_CMD},
    {"#", "!", "!", LVPRINT_COMMENT},
    {"ipv4.forwarding", "ip routing", "no ip routing", LVPRINT_CMD}, /* XXX: neg statement means to print it if not present */
    {"#", "!", "!", LVPRINT_COMMENT},
    
    {"interface[].unit[]", "interface $name", NULL, LVPRINT_MODE, ios_if_fmts},

    {"#", "!", "!", LVPRINT_COMMENT},
    {"router.router-id", "router-id $routerid", NULL, LVPRINT_CMD},
    {"#", "!", "!", LVPRINT_COMMENT},
    {"router.rip", "router rip", NULL, LVPRINT_MODE, ios_rip_fmts},
    {"#", "!", "!", LVPRINT_COMMENT},
    {"router.ospf", "router ospf", NULL, LVPRINT_MODE, ios_ospf_fmts},
    {"#", "!", "!", LVPRINT_COMMENT},
    {"router.bgp", "router bgp $as", NULL, LVPRINT_MODE, ios_bgp_fmts},
    {"#", "!", "!", LVPRINT_COMMENT},
    {"ipv4.route.static[]", "ip route $prefix $nexthop$distance{ $$}", NULL, LVPRINT_CMD},
    {"ipv4.route.static.null[]", "ip route $prefix$reject{ $$}$blackhole{ $$}$distance{ $$}", NULL, LVPRINT_CMD},
    {"#", "!", "!", LVPRINT_COMMENT},
    {"ipv4.arp[]", "arp $address $mac", NULL, LVPRINT_CMD},
    {"#", "!", "!", LVPRINT_COMMENT},
    {"access-list[]", "access-list $id $action$remark{ $$}$proto{ $$}$srcaddr{ $$}$srcmask{ $$}$dstaddr{ $$}$dstmask{ $$}", NULL, LVPRINT_CMD, "acl"},
    {"#", "!", "!", LVPRINT_COMMENT},
    {"ipv4.prefix-list[].description", "ip prefix-list $name description $description", NULL, LVPRINT_CMD},
    {"ipv4.prefix-list[].line[]", "ip prefix-list $name$seq{ seq $$}$action{ $$}$prefix{ $$}$ge{ ge $$}$le{ le $$}", NULL, LVPRINT_CMD},
    
    
    {"#", "!", "!", LVPRINT_COMMENT},

    {"ipv4.as-path.access-list[]", "ip as-path access-list $name $action $regexp", NULL},

    {"#", "!", "!", LVPRINT_COMMENT},
    {"route-map[].line[]", "route-map $name $action $line", NULL, LVPRINT_MODE, ios_route_map_fmts},
    {"#", "!", "!", LVPRINT_COMMENT},
    {"logging.buffered", "logging buffered$rows{ $$}$level{ $$}", NULL, LVPRINT_CMD},
    {"logging.trap", "logging trap $level", NULL, LVPRINT_CMD},
    {"logging.host[]", "logging host $host transport $protocol port $port", NULL, LVPRINT_CMD},

    {"snmp.community.ro[]", "snmp-server community $community ro", NULL, LVPRINT_CMD},
    {"snmp.community.rw[]", "snmp-server community $community rw", NULL, LVPRINT_CMD},
    {"snmp.contact", "snmp-server contact $contact", NULL, LVPRINT_CMD},
    {"snmp.location", "snmp-server location $location", NULL, LVPRINT_CMD},

    {"#", "!", "!", LVPRINT_COMMENT},
    {"ntp.logging", "ntp logging", NULL, LVPRINT_CMD},
    {"ntp.server[]", "ntp server $address", NULL, LVPRINT_CMD},
    {NULL, NULL}
};


/*
 * Show running config
 */
int
ios_show_running(clicon_handle h, cvec *vars, cg_var *arg)
{
    cli_ios_show_running(h, ios_fmts);
    return 0;
}

/*
 * Show startup configuration
 */
int
ios_show_startup(clicon_handle h, cvec *vars, cg_var *arg)
{
    cli_ios_show_config(h, clicon_startup_config(h), ios_fmts);
    return 0;
}

/*
 * Show startup configuration
 */
int
ios_show_archive_diff(clicon_handle h, cvec *vars, cg_var *arg)
{
    cg_var *cv1 = cvec_i(vars, 1);

    ios_cli_show_archive_diff(h, cv_string_get(cv1), ios_fmts);
    return 0;
}

