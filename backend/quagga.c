/*
 *  CVS Version: $Id: quagga.c,v 1.26 2013/08/09 13:27:46 olof Exp $
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
 * * 
 */
#ifdef HAVE_ROST_CONFIG_H
#include "rost_config.h" /* generated by config & autoconf */
#endif /* HAVE_ROST_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <getopt.h>

/* clicon */
#include <cligen/cligen.h>
#include <clicon/clicon.h>
#include <clicon/clicon_backend.h>

/* lib */
#include "quaggapi.h"


struct qaction {
    char *key;
    char *cmd;
    char *nocmd;
    char *apipath;
};
static struct qaction qactions[] = {
    {
	"system.hostname",
	"hostname $hostname",
	"no hostname $hostname",
	ALL_API_SOCK
    },

    {
	"ipv4.forwarding",
	"ip forwarding", 
	"no ip forwarding",
	ZEBRA_API_SOCK
    },
    
    {
	"interface[].unit[]",
	"interface $name",
	"no interface $name",
	ZEBRA_API_SOCK
    },
    
    {
	"interface[].unit[].description",
	"interface $name\n description $description",
	"interface $name\n no description",
	ZEBRA_API_SOCK
    },

    {
	"interface[].unit[].inet.address[]",
	"interface $name\n ip address $prefix",
	"interface $name\n no ip address $prefix",
	ZEBRA_API_SOCK
    },

    {
	"interface[].unit[].ipv6.address[]",
	"interface $name\n ipv6 address $prefix",
	"interface $name\n no ipv6 address $prefix",
	ZEBRA_API_SOCK
    },

    {
	"interface[].unit[].bandwidth",
	"interface $name\n bandwidth $bandwidth",
	"interface $name\n no bandwidth $bandwidth",
	ZEBRA_API_SOCK
    },

    {
	"interface[].unit[].link-detect",
	"interface $name\n link-detect",
	"interface $name\n no link-detect",
	ZEBRA_API_SOCK
    },

    {
	"interface[].unit[].shutdown",
	"interface $name\n shutdown",
	"interface $name\n no shutdown",
	ZEBRA_API_SOCK
    },

    {
	"interface[].unit[].ospf.cost",
	"interface $name\n ip ospf cost $cost",
	"interface $name\n no ip ospf cost $cost",
	OSPF_API_SOCK
    },

    {
	"interface[].unit[].ospf.network-type",
	"interface $name\n ip ospf network-type $network_type",
	"interface $name\n no ip ospf network-type $network_type",
	OSPF_API_SOCK
    },

    {
	"interface[].unit[].ospf.priority",
	"interface $name\n ip ospf priority $priority",
	"interface $name\n no ip ospf priority $priority",
	OSPF_API_SOCK
    },

    {
	"interface[].unit[].rip.receive",
	"interface $name\n ip rip receive version $version",
	"interface $name\n no ip rip receive version",
	RIP_API_SOCK
    },

    {
	"interface[].unit[].rip.send",
	"interface $name\n ip rip send version $version",
	"interface $name\n no ip rip send version",
	RIP_API_SOCK
    },

    {
	"interface[].unit[].rip.split-horizon",
	"interface $name$no{\nno ip rip split-horizon}$poisoned_reverse{\nip rip split-horizon $$}",
	"interface $name$no{\nip rip split-horizon}$poisoned_reverse{\nno ip rip split-horizon $$}",
	RIP_API_SOCK
    },

    {
	"router.router-id",
	"router-id $routerid",
	"no router-id",
	ZEBRA_API_SOCK
    },

    {
	"router.bgp",
	"router bgp $as",
	"no router bgp $as",
	BGP_API_SOCK
    },

    {
	"router.bgp.router-id",
	"router bgp $router.bgp:as\n bgp router-id $routerid",
	"router bgp $router.bgp:as\n no bgp router-id $routerid",
	BGP_API_SOCK
    },

    {
	"router.bgp.log-neighbor-changes",
	"router bgp $router.bgp:as\n bgp log-neighbor-changes",
	"router bgp $router.bgp:as\n no bgp log-neighbor-changes",
	BGP_API_SOCK
    },

    {
	"router.bgp.always-compare-med",
	"router bgp $router.bgp:as\n bgp always-compare-med",
	"router bgp $router.bgp:as\n no bgp always-compare-med",
	BGP_API_SOCK
    },

    {
	"router.bgp.default.local-preference",
	"router bgp $router.bgp:as\n bgp default local-preference $local_preference",
	"router bgp $router.bgp:as\n no bgp default local-preference $local_preference",
	BGP_API_SOCK
    },

    {
	"router.bgp.enforce-first-as",
	"router bgp $router.bgp:as\n bgp enforce-first-as",
	"router bgp $router.bgp:as\n no bgp enforce-first-as",
	BGP_API_SOCK
    },

    {
	"router.bgp.deterministic-med",
	"router bgp $router.bgp:as\n bgp deterministic-med",
	"router bgp $router.bgp:as\n no bgp deterministic-med",
	BGP_API_SOCK
    },

    {
	"router.bgp.graceful-restart",
	"router bgp $router.bgp:as\n bgp graceful-restart$stalepath_time{ stalepath-time $$}",
	"router bgp $router.bgp:as\n no bgp graceful-restart$stalepath_time{ stalepath-time $$}",
	BGP_API_SOCK
    },

    {
	"router.bgp.network.import-check",
	"router bgp $router.bgp:as\n bgp network import-check",
	"router bgp $router.bgp:as\n no network import-check",
	BGP_API_SOCK
    },

    {
	"router.bgp.dampening",
	"router bgp $router.bgp:as\n bgp dampening$half_life{ $$}$reuse{ $$}$suppress{ $$}$max_suppress{ $$}",
	"router bgp $router.bgp:as\n no bgp dampening$half_life{ $$}$reuse{ $$}$suppress{ $$}$max_suppress{ $$}",
	BGP_API_SOCK
    },

    {
	"router.bgp.network[]",
	"router bgp $router.bgp:as\n network $prefix$route_map{ route-map $$}",
	"router bgp $router.bgp:as\n no network $prefix$route_map{ route-map $$}",
	BGP_API_SOCK
    },

    {
	"router.bgp.aggregate-address[]",
	"router bgp $router.bgp:as\n aggregate-address $prefix$as_set{ $$}$summary_only{ $$}",
	"router bgp $router.bgp:as\n no aggregate-address $prefix$as_set{ $$}$summary_only{ $$}",
	BGP_API_SOCK
    },
    
    {
	"router.bgp.redistribute.connected",
	"router bgp $router.bgp:as\n redistribute connected $metric{ metric $$}$route_map{ route-map $$}",
	"router bgp $router.bgp:as\n no redistribute connected",
	BGP_API_SOCK
    },

    {
	"router.bgp.redistribute.kernel",
	"router bgp $router.bgp:as\n redistribute kernel $metric{ metric $$}$route_map{ route-map $$}",
	"router bgp $router.bgp:as\n no redistribute kernel",
	BGP_API_SOCK
    },

    {
	"router.bgp.redistribute.ospf",
	"router bgp $router.bgp:as\n redistribute ospf $metric{ metric $$}$route_map{ route-map $$}",
	"router bgp $router.bgp:as\n no redistribute ospf",
	BGP_API_SOCK
    },

    {
	"router.bgp.redistribute.rip",
	"router bgp $router.bgp:as\n redistribute rip $metric{ metric $$}$route_map{ route-map $$}",
	"router bgp $router.bgp:as\n no redistribute rip",
	BGP_API_SOCK
    },

    {
	"router.bgp.redistribute.static",
	"router bgp $router.bgp:as\n redistribute static $metric{ metric $$}$route_map{ route-map $$}",
	"router bgp $router.bgp:as\n no redistribute static",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].remote-as",
	"router bgp $router.bgp:as\n neighbor $neighbor remote-as $remote_as",
	"router bgp $router.bgp:as\n no neighbor $neighbor remote-as $remote_as",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].local-as",
	"router bgp $router.bgp:as\n neighbor $neighbor local-as $localas$no_prepend{ $$}",
	"router bgp $router.bgp:as\n no neighbor $neighbor local-as $localas$no_prepend{ $$}",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].description",
	"router bgp $router.bgp:as\n neighbor $neighbor description $description",
	"router bgp $router.bgp:as\n no neighbor $neighbor description $description",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].shutdown",
	"router bgp $router.bgp:as\n neighbor $neighbor shutdown",
	"router bgp $router.bgp:as\n no neighbor $neighbor shutdown",
	BGP_API_SOCK
    },
    
    {
	"router.bgp.neighbor[].password",
	"router bgp $router.bgp:as\n neighbor $neighbor password $password",
	"router bgp $router.bgp:as\n no neighbor $neighbor password $password",
	BGP_API_SOCK
    },
    
    {
	"router.bgp.neighbor[].passive",
	"router bgp $router.bgp:as\n neighbor $neighbor passive",
	"router bgp $router.bgp:as\n no neighbor $neighbor passive",
	BGP_API_SOCK
    },
    
    {
	"router.bgp.neighbor[].ebgp-multihop",
	"router bgp $router.bgp:as\n neighbor $neighbor ebgp-multihop$maxhops{ $$}",
	"router bgp $router.bgp:as\n no neighbor $neighbor ebgp-multihop$maxhops{ $$}",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].update-source",
	"router bgp $router.bgp:as\n neighbor $neighbor update-source $source",
	"router bgp $router.bgp:as\n no neighbor $neighbor update-source",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].advertisement-interval",
	"router bgp $router.bgp:as\n neighbor $neighbor advertisement-interval $interval",
	"router bgp $router.bgp:as\n no neighbor $neighbor advertisement-interval $interval",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].timers",
	"router bgp $router.bgp:as\n neighbor $neighbor timers $keepalive $holdtime",
	"router bgp $router.bgp:as\n no neighbor $neighbor timers $keepalive $holdtime",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].timers.connect",
	"router bgp $router.bgp:as\n neighbor $neighbor timers connect $connect",
	"router bgp $router.bgp:as\n no neighbor $neighbor timers connect $connect",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].weight",
	"router bgp $router.bgp:as\n neighbor $neighbor weight $weight",
	"router bgp $router.bgp:as\n no neighbor $neighbor weight $weight",
	BGP_API_SOCK
    },
    
    {
	"router.bgp.neighbor[].next-hop-self",
	"router bgp $router.bgp:as\n neighbor $neighbor next-hop-self",
	"router bgp $router.bgp:as\n no neighbor $neighbor next-hop-self",
	BGP_API_SOCK
    },
    
    {
	"router.bgp.neighbor[].remove-private-as",
	"router bgp $router.bgp:as\n neighbor $neighbor remove-private-AS",
	"router bgp $router.bgp:as\n no neighbor $neighbor remove-private-AS",
	BGP_API_SOCK
    },
    
    {
	"router.bgp.neighbor[].default-originate",
	"router bgp $router.bgp:as\n neighbor $neighbor default-originate$route_map{ route-map $$}",
	"router bgp $router.bgp:as\n no neighbor $neighbor default-originate$route_map{ route-map $$}",
	BGP_API_SOCK
    },
    
    {
	"router.bgp.neighbor[].soft-reconfiguration.inbound",
	"router bgp $router.bgp:as\n neighbor $neighbor soft-reconfiguration inbound",
	"router bgp $router.bgp:as\n no neighbor $neighbor soft-reconfiguration inbound",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].prefix-list.in",
	"router bgp $router.bgp:as\n neighbor $neighbor prefix-list $prefix_list in",
	"router bgp $router.bgp:as\n no neighbor $neighbor prefix-list $prefix_list in",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].prefix-list.out",
	"router bgp $router.bgp:as\n neighbor $neighbor prefix-list $prefix_list out",
	"router bgp $router.bgp:as\n no neighbor $neighbor prefix-list $prefix_list out",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].route-map.in",
	"router bgp $router.bgp:as\n neighbor $neighbor route-map $route_map in",
	"router bgp $router.bgp:as\n no neighbor $neighbor route-map $route_map in",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].route-map.out",
	"router bgp $router.bgp:as\n neighbor $neighbor route-map $route_map out",
	"router bgp $router.bgp:as\n no neighbor $neighbor route-map $route_map out",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].filter-list.in",
	"router bgp $router.bgp:as\n neighbor $neighbor filter-list $acl in",
	"router bgp $router.bgp:as\n no neighbor $neighbor filter-list $acl in",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].filter-list.out",
	"router bgp $router.bgp:as\n neighbor $neighbor filter-list $acl out",
	"router bgp $router.bgp:as\n no neighbor $neighbor filter-list $acl out",
	BGP_API_SOCK
    },

    {
	"router.bgp.neighbor[].activate",
	"router bgp $router.bgp:as\n neighbor $neighbor activate",
	"router bgp $router.bgp:as\n no neighbor $neighbor activate",
	BGP_API_SOCK
    },

    {
	"router.bgp.distance.bgp",
	"router bgp $router.bgp:as\n distance bgp $external $internal $local",
	"router bgp $router.bgp:as\n no distance bgp $external $internal $local",
	BGP_API_SOCK
    },

    {
	"router.bgp.distance[]",
	"router bgp $router.bgp:as\n distance $distance $prefix$acl{ $$}",
	"router bgp $router.bgp:as\n no distance $distance $prefix$acl{ $$}",
	BGP_API_SOCK
    },

    {
	"router.ospf.router-id",
	"router ospf\n ospf router-id $routerid",
	"router ospf\n no ospf router-id",
	OSPF_API_SOCK
    },

    {
	"router.ospf.abr-type",
	"router ospf\n ospf abr-type $abr_type",
	"router ospf\n no ospf abr-type $abr_type",
	OSPF_API_SOCK
    },

    {
	"router.ospf.log-adjacency-changes",
	"router ospf\n log-adjacency-changes$detail{ $$}",
	"router ospf\n no log-adjacency-changes$detail{ $$}",
	OSPF_API_SOCK
    },

    {
	"router.ospf.compatible",
	"router ospf\n compatible rfc1583",
	"router ospf\n no compatible rfc1583",
	OSPF_API_SOCK
    },

    {
	"router.ospf.auto-cost.reference-bandwidth",
	"router ospf\n auto-cost reference-bandwidth $bandwidth",
	"router ospf\n noauto-cost reference-bandwidth $bandwidth",
	OSPF_API_SOCK
    },

    {
	"router.ospf.timers.throttle.spf",
	"router ospf\n timers throttle spf $delay $initial_hold $max_hold",
	"router ospf\n no timers throttle spf $delay $initial_hold $max_hold",
	OSPF_API_SOCK
    },

    {
	"router.ospf.max-metric.router-lsa.on-startup",
	"router ospf\n max-metric router-lsa on-startup $on_startup",
	"router ospf\n no max-metric router-lsa on-startup",
	OSPF_API_SOCK
    },

    {
	"router.ospf.max-metric.router-lsa.on-shutdown",
	"router ospf\n max-metric router-lsa on-shutdown $on_shutdown",
	"router ospf\n no max-metric router-lsa on-shutdown",
	OSPF_API_SOCK
    },

    {
	"router.ospf.max-metric.router-lsa.administrative",
	"router ospf\n max-metric router-lsa administrative",
	"router ospf\n no max-metric router-lsa administrative",
	OSPF_API_SOCK
    },

    {
	"router.ospf.refresh-timer",
	"router ospf\n refresh timer $timer",
	"router ospf\n no refresh timer $timer",
	OSPF_API_SOCK
    },

    {
	"router.ospf.redistribute.bgp",
	"router ospf\n redistribute bgp $metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}",
	"router ospf\n no redistribute bgp",
	OSPF_API_SOCK
    },

    {
	"router.ospf.redistribute.connected",
	"router ospf\n redistribute connected $metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}",
	"router ospf\n no redistribute connected",
	OSPF_API_SOCK
    },

    {
	"router.ospf.redistribute.isis",
	"router ospf\n redistribute isis $metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}",
	"router ospf\n no redistribute isis",
	OSPF_API_SOCK
    },

    {
	"router.ospf.redistribute.kernel",
	"router ospf\n redistribute kernel $metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}",
	"router ospf\n no redistribute kernel",
	OSPF_API_SOCK
    },

    {
	"router.ospf.redistribute.rip",
	"router ospf\n redistribute rip $metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}",
	"router ospf\n no redistribute rip",
	OSPF_API_SOCK
    },

    {
	"router.ospf.redistribute.static",
	"router ospf\n redistribute static $metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}",
	"router ospf\n no redistribute static",
	OSPF_API_SOCK
    },

    {
	"router.ospf.passive-interface[",
	"router ospf\n passive-interface $interface",
	"router ospf\n no passive-interface $interface",
	OSPF_API_SOCK
    },

    {
	"router.ospf.network[]",
	"router ospf\n network $prefix area $area",
	"router ospf\n no network $prefix area $area",
	OSPF_API_SOCK
    },

    {
	"router.ospf.area[].auth",
	"router ospf\n area $area authentication$auth{ $$}",
	"router ospf\n no area $area authentication$auth{ $$}",
	OSPF_API_SOCK
    },

    {
	"router.ospf.area[].shortcut",
	"router ospf\n area $area shortcut $mode",
	"router ospf\n no area $area shortcut $mode",
	OSPF_API_SOCK
    },

    {
	"router.ospf.area[].stub",
	"router ospf\n area $area stub$no_summary{ $$}",
	"router ospf\n no area $area stub$no_summary{ $$}",
	OSPF_API_SOCK
    },

    {
	"router.ospf.area[].default-cost",
	"router ospf\n area $area default-cost $cost",
	"router ospf\n no area $area default-cost $cost",
	OSPF_API_SOCK
    },

    {
	"router.ospf.area[].export-list",
	"router ospf\n area $area export-list $export_list",
	"router ospf\n no area $area export-list $export_list",
	OSPF_API_SOCK
    },

    {
	"router.ospf.area[].import-list",
	"router ospf\n area $area import-list $import_list",
	"router ospf\n no area $area import-list $import_list",
	OSPF_API_SOCK
    },

    {
	"router.ospf.area[].nssa",
	"router ospf\n area $area nssa$translate{ translate-$$}$no_summary{ $$}",
	"router ospf\n no area $area nssa$translate{ translate-$$}$no_summary{ $$}",
	OSPF_API_SOCK
    },

    {
	"router.ospf.area[].range",
	"router ospf\n area $area range $range$substitute{ substitute $$}$not_advertise{ $$}$cost{ cost $$}",
	"router ospf\n no area $area range $range$substitute{ substitute $$}$not_advertise{ $$}$cost{ cost $$}",
	OSPF_API_SOCK
    },

    {
	"router.ospf.neighbor[]",
	"router ospf\n neighbor $neighbor$priority{ priority $$}$poll_interval{ poll-interval $$}",
	"router ospf\n no neighbor $neighbor$priority{ priority $$}$poll_interval{ poll-interval $$}",
	OSPF_API_SOCK
    },

    {
	"router.ospf.default-metric",
	"router ospf\n default-metric $metric",
	"router ospf\n no default-metric $metric",
	OSPF_API_SOCK
    },

    {
	"router.ospf.distribute-list[]",
	"router ospf\n distribute-list $acl $direction $protocol",
	"router ospf\n no distribute-list $acl $direction $protocol",
	OSPF_API_SOCK
    },

    {
	"router.ospf.default-information.originate",
	"router ospf\n default-information originate$always{ $$}$metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}",
	"router ospf\n no default-information originate$always{ $$}$metric{ metric $$}$metric_type{ metric-type $$}$route_map{ route-map $$}",
	OSPF_API_SOCK
    },

    {
	"router.ospf.distance",
	"router ospf\n distance $distance",
	"router ospf\n no distance $distance",
	OSPF_API_SOCK
    },

    {
	"router.ospf.distance.ospf",
	"router ospf\n distance ospf$intra_area{ intra-area $$}$inter_area{ inter-area $$}$external{ external $$}",
	"router ospf\n no distance ospf$intra_area{ intra-area $$}$inter_area{ inter-area $$}$external{ external $$}",
	OSPF_API_SOCK
    },

    {
	"router.ospf.capability",
	"router ospf\n capability opaque",
	"router ospf\n no capability opaque",
	OSPF_API_SOCK
    },

    {
	"router.ospf",
	"router ospf",
	"no router ospf",
	OSPF_API_SOCK
    },

    {
	"router.rip.version",
	"router rip\nversion $version",
	"router rip\nno version",
	RIP_API_SOCK
    },

    {
	"router.rip.timers.basic",
	"router rip\ntimers basic $update $timeout $garbage",
	"router rip\nno timers basic",
	RIP_API_SOCK
    },

    {
	"router.rip.default-information",
	"router rip\ndefault-information originate",
	"router rip\nno default-information originate",
	RIP_API_SOCK
    },

    {
	"router.rip.redistribute.bgp",
	"router rip\n redistribute bgp $metric{ metric $$}$route_map{ route-map $$}",
	"router rip\n no redistribute bgp",
	RIP_API_SOCK
    },

    {
	"router.rip.redistribute.connected",
	"router rip\n redistribute connected $metric{ metric $$}$route_map{ route-map $$}",
	"router rip\n no redistribute connected",
	RIP_API_SOCK
    },

    {
	"router.rip.redistribute.isis",
	"router rip\n redistribute isis $metric{ metric $$}$route_map{ route-map $$}",
	"router rip\n no redistribute isis",
	RIP_API_SOCK
    },

    {
	"router.rip.redistribute.kernel",
	"router rip\n redistribute kernel $metric{ metric $$}$route_map{ route-map $$}",
	"router rip\n no redistribute kernel",
	RIP_API_SOCK
    },

    {
	"router.rip.redistribute.ospf",
	"router rip\n redistribute ospf $metric{ metric $$}$route_map{ route-map $$}",
	"router rip\n no redistribute ospf",
	RIP_API_SOCK
    },

    {
	"router.rip.redistribute.static",
	"router rip\n redistribute static $metric{ metric $$}$route_map{ route-map $$}",
	"router rip\n no redistribute static",
	RIP_API_SOCK
    },

    {
	"router.rip.offset-list[]",
	"router rip\noffset-list $acl $direction $metric$interface{ $$}",
	"router rip\nno offset-list $acl $direction $metric$interface{ $$}",
	RIP_API_SOCK
    },

    {
	"router.rip.network.prefix[]",
	"router rip\nnetwork $prefix",
	"router rip\nno network $prefix",
	RIP_API_SOCK
    },

    {
	"router.rip.network.interface[",
	"router rip\nnetwork $interface",
	"router rip\nno network $interface",
	RIP_API_SOCK
    },

    {
	"router.rip.neighbor[]",
	"router rip\nneighbor $neighbor",
	"router rip\nno neighbor $neighbor",
	RIP_API_SOCK
    },

    {
	"router.rip.passive-interface[].unit[]",
	"router rip\npassive-interface $interface",
	"router rip\nno passive-interface $interface",
	RIP_API_SOCK
    },

    {
	"router.rip.default-metric",
	"router rip\ndefault-metric $metric",
	"router rip\nno default-metric",
	RIP_API_SOCK
    },

    {
	"router.rip.distribute-list[]",
	"router rip\ndistribute-list $acl $direction$interface{ $$}",
	"router rip\nno distribute-list $acl $direction$interface{ $$}",
	RIP_API_SOCK
    },

    {
	"router.rip.distribute-list.prefix[]",
	"router rip\ndistribute-list prefix $prefix $direction$interface{ $$}",
	"router rip\nno distribute-list prefix $prefix $direction$interface{ $$}",
	RIP_API_SOCK
    },

    {
	"router.rip.route-map[]",
	"router rip\nroute-map $route_map $direction $interface",
	"router rip\nno route-map $route_map $direction $interface",
	RIP_API_SOCK
    },

    {
	"router.rip.distance",
	"router rip\ndistance $distance",
	"router rip\nno distance $distance",
	RIP_API_SOCK
    },

    {
	"router.rip.distance.prefix[]",
	"router rip\ndistance $distance $prefix $acl{ $$}",
	"router rip\nno distance $distance $prefix",
	RIP_API_SOCK
    },

    {
	"router.rip",
	"router rip",
	"no router rip",
	RIP_API_SOCK
    },

    {
	"ipv4.route.static[]",
	"ip route $prefix $nexthop$distance{ $$}",
	"no ip route $prefix $nexthop$distance{ $$}",
	ZEBRA_API_SOCK
    },

    {
	"ipv4.route.static.null[]",
	"ip route $prefix$reject{ $$}$blackhole{ $$}$distance{ $$}",
	"no ip route $prefix$reject{ $$}$blackhole{ $$}$distance{ $$}",
	ZEBRA_API_SOCK
    },

    {
	"access-list[]",
	"access-list $id $action$remark{ $$}$proto{ $$}$srcaddr{ $$}$srcmask{ $$}$dstaddr{ $$}$dstmask{ $$}",
	"no access-list $id $action$remark{ $$}$proto{ $$}$srcaddr{ $$}$srcmask{ $$}$dstaddr{ $$}$dstmask{ $$}",
	ALL_API_SOCK 
    },

    {
	"ipv4.prefix-list[].description",
	"ip prefix-list $name description $description",
	"no ip prefix-list $name $description",
	ALL_API_SOCK 
    },

    {
	"ipv4.prefix-list[].line[]",
	"ip prefix-list $name $action $prefix $ge{ ge $$}$le{ le $$}",
	"no ip prefix-list $name $action $prefix $ge{ ge $$}$le{ le $$}",
	ALL_API_SOCK 
    },

    {
	"route-map[].line[]",
	"route-map $name $action $line",
	"no route-map $name $action $line",
	ALL_API_SOCK
    },

    {
	"route-map[].line[].call",
	"route-map $name $action $line\n call $route_map",
	"route-map $name $action $line\n no call $route_map",
	ALL_API_SOCK
    },

    {
	"route-map[].line[].description",
	"route-map $name $action $line\n description $description",
	"route-map $name $action $line\n no description $description",
	ALL_API_SOCK
    },

    {
	"route-map[].line[].match.as-path",
	"route-map $name $action $line\n match as-path $as_path",
	"route-map $name $action $line\n no match as-path$as_path{ $$}",
	BGP_API_SOCK
    },

    {
	"route-map[].line[].match.interface",
	"route-map $name $action $line\n match interface $interface",
	"route-map $name $action $line\n no match interface$interface{ $$}",
	ZEBRA_API_SOCK ":" OSPF_API_SOCK ":" RIP_API_SOCK 
    },

    {
	"route-map[].line[].match.ip.address",
	"route-map $name $action $line\n match ip address $acl",
	"route-map $name $action $line\n no match ip address$acl{ $$}",
	ALL_API_SOCK
    },

    {
	"route-map[].line[].match.ip.address.prefix-list",
	"route-map $name $action $line\n match ip address prefix-list $prefix_list",
	"route-map $name $action $line\n no match ip address prefix-list$prefix_list{ $$}",
	ALL_API_SOCK
    },

    {
	"route-map[].line[].match.ip.next-hop",
	"route-map $name $action $line\n match ip next-hop $acl",
	"route-map $name $action $line\n no match ip next-hop$acl{ $$}",
	ALL_API_SOCK
    },

    {
	"route-map[].line[].match.ip.next-hop.prefix-list",
	"route-map $name $action $line\n match ip next-hop prefix-list$prefix_list",
	"route-map $name $action $line\n no match ip next-hop prefix-list$prefix_list{ $$}",
	ALL_API_SOCK
    },

    {
	"route-map[].line[].match.ip.route-source",
	"route-map $name $action $line\n match ip route-source $acl",
	"route-map $name $action $line\n no match ip route-source$acl{ $$}",
	BGP_API_SOCK
    },

    {
	"route-map[].line[].match.ip.route-source.prefix-list",
	"route-map $name $action $line\n match ip route-source prefix-list$prefix_list",
	"route-map $name $action $line\n no match ip route-source prefix-list$prefix_list{ $$}",
	ALL_API_SOCK
    },

    {
	"route-map[].line[].match.metric",
	"route-map $name $action $line\n match metric $metric",
	"route-map $name $action $line\n no match metric$metric{ $$}",
	RIP_API_SOCK ":" BGP_API_SOCK
    },

    {
	"route-map[].line[].on-match",
	"route-map $name $action $line\n on-match $op$goto{ $$}",
	"route-map $name $action $line\n no on-match $op",
	ALL_API_SOCK
    },

    {
	"route-map[].line[].set.as-path.exclude",
	"route-map $name $action $line\n set as-path exclude $as_path",
	"route-map $name $action $line\n no set as-path exclude$as_path{ $$}",
	BGP_API_SOCK
    },

    {
	"route-map[].line[].set.as-path.prepend",
	"route-map $name $action $line\n set as-path prepend $as_path",
	"route-map $name $action $line\n no set as-path prepend$as_path{ $$}",
	BGP_API_SOCK
    },

    {
	"route-map[].line[].set.ip.next-hop",
	"route-map $name $action $line\n set ip next-hop $nexthop",
	"route-map $name $action $line\n no set ip next-hop$nexthop{ $$}",
	BGP_API_SOCK
    },

    {
	"route-map[].line[].set.local-preference",
	"route-map $name $action $line\n set local-preference $localpref",
	"route-map $name $action $line\n no set local-preference$localpref{ $$}",
	BGP_API_SOCK
    },

    {
	"route-map[].line[].set.metric",
	"route-map $name $action $line\n set metric$op{ $$} $metric",
	"route-map $name $action $line\n no set metric$op{ $$}$metric{ $$}",
	RIP_API_SOCK ":" BGP_API_SOCK
    },

    {
	"route-map[].line[].set.metric-type",
	"route-map $name $action $line\n set metric-type $metric_type",
	"route-map $name $action $line\n no set metric-type$metric_type{ $$}",
	OSPF_API_SOCK
    },

    {
	"route-map[].line[].set.origin",
	"route-map $name $action $line\n set origin $origin",
	"route-map $name $action $line\n no set origin$origin{ $$}",
	BGP_API_SOCK
    },

    {
	"route-map[].line[].set.originator-id",
	"route-map $name $action $line\n set originator-id $originator_id",
	"route-map $name $action $line\n no set originator-id$originator_id{ $$}",
	BGP_API_SOCK
    },

    {
	"route-map[].line[].set.pathlimit.ttl",
	"route-map $name $action $line\n set pathlimit ttl $ttl",
	"route-map $name $action $line\n no set pathlimit ttl$ttl{ $$}",
	BGP_API_SOCK
    },

    {
	"route-map[].line[].set.tag",
	"route-map $name $action $line\n set tag $route_tag",
	"route-map $name $action $line\n no set tag$route_tag{ $$}",
	RIP_API_SOCK
    },

    {
	"route-map[].line[].set.weight",
	"route-map $name $action $line\n set weight $weight",
	"route-map $name $action $line\n no set weight$weight{ $$}",
	BGP_API_SOCK
    },

    {
	"route-map[].line[].continue",
	"route-map $name $action $line\n continue$goto{ $$}",
	"route-map $name $action $line\n no continue$goto{ $$}",
	ALL_API_SOCK
    },

    {NULL, NULL, NULL, NULL},
};

/*
 * lv_quagga_exec
 */
static int
lv_quagga_exec(char *cmd, char *sockpath)
{
    int retcode;
    int retval = -1;
    char *output = NULL;
    struct quaggapi_batch *batch = NULL;

    batch = quaggapi_strexec(sockpath, 0, "configure terminal\n%s", cmd);
    if (batch == NULL)
	goto catch;
    if (batch->numexec < 0){ /* e.g socket error - no contact w quagga */
	if (!clicon_errno)
	    clicon_err(OE_ROUTING, 0, "No contact with quagga");
	goto catch;
    }
    output = batch->cmds[batch->numexec-1].output;
    retcode = batch->cmds[batch->numexec-1].retcode;

    /* Dont error, pass as an exception upwards */
    if (retcode == QUAGGA_SUCCESS) 
	; /* do nothing for now */
    else{ 
	clicon_err(OE_ROUTING, retcode, "Quagga error: %s", output);
	goto catch;
    }
    retval = 0;
catch:
    if (batch)
	quaggapi_free (batch);
    return retval;
}




static int
quagga_exec(clicon_handle h, char *cmd, char *apipath)
{
    int i;
    int ret = -1;
    int nvec;
    char **vec;
    char path[PATH_MAX];
    char *dir;

    /* Get Quagga socket directory */
    if ((dir = clicon_option_str(h, "QUAGGA_DIR")) == NULL)
	dir = QUAGGA_DIR;

    /* Split apipath and call exec for each daemon */
    if ((vec = clicon_strsplit(apipath, ":", &nvec, NULL))) {
	for (i=0; i < nvec; i++) { 
	    snprintf(path, sizeof(path), "%s/%s", dir, vec[i]);
	    if ((ret = lv_quagga_exec(cmd, path)) < 0)
		goto catch;
	}
    }
    
catch:
    unchunk (vec);
    return ret;
}

/* Commit callback */
int
quagga_commit(clicon_handle h, char *db,
	      trans_cb_type tt, 
	      lv_op_t op,
	      char *key,
	      void *arg)
{
    int retval = -1;
    char *cmd;
    struct qaction *qa = (struct qaction *)arg;

    if (op == LV_SET)
	cmd = lvmap_fmt(db, qa->cmd, key);
    else
	cmd = lvmap_fmt(db, qa->nocmd, key);
    if (cmd == NULL)
	return -1;

    if (debug)
	clicon_log(LOG_DEBUG, "%s: %s: %s\n",  __FUNCTION__, key, cmd);

    retval = quagga_exec(h, cmd, qa->apipath);
    free(cmd);
    
    return retval;
}


/*
 * Plugin initialization
 */
int
plugin_init(clicon_handle h)
{
    int i;
    char *key;
    dbdep_handle_t *d;
    int retval = -1;

    for (i = 0; qactions[i].key; i++) {
	key = qactions[i].key;
	if ((d=dbdep(h, TRANS_CB_COMMIT, quagga_commit, (void *)&qactions[i], 0))==NULL){
	    clicon_debug(1, "Failed to create dependency '%s'", key);
	    goto done;
	}
	
	if (dbdep_ent(d, key, NULL) < 0)  {
	    clicon_debug(1, "Failed to create dependency '%s'", key);
	    goto done;
	}
	clicon_debug(1, "Created dependency '%s'", key);
    }
    retval = 0;
  done:
    return retval;
}

/*
 * Plugin start
 */
int
plugin_start(clicon_handle h, int argc, char **argv)
{
    char c;

    optind = 0;
    opterr = 0;
    while ((c = getopt(argc, argv, "q:")) != -1) {
	switch (c) {
	case 'q' : /* debug */
	    if (optarg && strlen(optarg))
		clicon_option_str_set(h, "QUAGGA_DIR", optarg);
	    break;
	}
    }

    /* Enable quagga logging to syslog */
    if (quagga_exec (h, "log syslog debugging", ALL_API_SOCK) < 0)
	return -1;
    
    return 0;
}

