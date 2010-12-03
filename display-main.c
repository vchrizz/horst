/******************* MAIN / OVERVIEW *******************/

#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "main.h"
#include "util.h"
#include "ieee80211.h"
#include "olsr_header.h"
#include "listsort.h"


static WINDOW *sort_win = NULL;
static WINDOW *dump_win = NULL;
static WINDOW *list_win = NULL;
static WINDOW *stat_win = NULL;

static int do_sort = 'n';
/* pointer to the sort function */
static int(*sortfunc)(const struct list_head*, const struct list_head*) = NULL;

/* sizes of split window (list_win & status_win) */
static int win_split;
static int stat_height;


/******************* SORTING *******************/

static int
compare_nodes_snr(const struct list_head *p1, const struct list_head *p2)
{
	struct node_info* n1 = list_entry(p1, struct node_info, list);
	struct node_info* n2 = list_entry(p2, struct node_info, list);

	if (n1->last_pkt.phy_snr > n2->last_pkt.phy_snr)
		return -1;
	else if (n1->last_pkt.phy_snr == n2->last_pkt.phy_snr)
		return 0;
	else
		return 1;
}


static int
compare_nodes_time(const struct list_head *p1, const struct list_head *p2)
{
	struct node_info* n1 = list_entry(p1, struct node_info, list);
	struct node_info* n2 = list_entry(p2, struct node_info, list);

	if (n1->last_seen > n2->last_seen)
		return -1;
	else if (n1->last_seen == n2->last_seen)
		return 0;
	else
		return 1;
}


static int
compare_nodes_channel(const struct list_head *p1, const struct list_head *p2)
{
	struct node_info* n1 = list_entry(p1, struct node_info, list);
	struct node_info* n2 = list_entry(p2, struct node_info, list);

	if (n1->wlan_channel < n2->wlan_channel)
		return 1;
	else if (n1->wlan_channel == n2->wlan_channel)
		return 0;
	else
		return -1;
}


static int
compare_nodes_bssid(const struct list_head *p1, const struct list_head *p2)
{
	struct node_info* n1 = list_entry(p1, struct node_info, list);
	struct node_info* n2 = list_entry(p2, struct node_info, list);

	return -memcmp(n1->wlan_bssid, n2->wlan_bssid, MAC_LEN);
}


static void
sort_input(int c)
{
	switch (c) {
	case 'n': case 'N': sortfunc = NULL; break;
	case 's': case 'S': sortfunc = compare_nodes_snr; break;
	case 't': case 'T': sortfunc = compare_nodes_time; break;
	case 'c': case 'C': sortfunc = compare_nodes_channel; break;
	case 'b': case 'B': sortfunc = compare_nodes_bssid; break;
	}

	switch (c) {
	case 'n': case 'N':
	case 's': case 'S':
	case 't': case 'T':
	case 'c': case 'C':
	case 'b': case 'B':
		do_sort = c;
		/* fall thru */
	case '\r': case KEY_ENTER:
		delwin(sort_win);
		sort_win = NULL;
		update_display(NULL, NULL);
		return;
	case 'q': case 'Q':
		finish_all(0);
	}
}


static void
show_sort_win(void)
{
	if (sort_win == NULL) {
		sort_win = newwin(1, COLS-2, win_split - 2, 1);
		wattron(sort_win, BLACKONWHITE);
		mvwhline(sort_win, 0, 0, ' ', COLS);
		mvwprintw(sort_win, 0, 0, " -> Sort by s:SNR t:Time b:BSSID c:Channel n:Don't sort [current: %c]", do_sort);
		wrefresh(sort_win);
	}
}


/******************* WINDOWS *******************/

static void
update_status_win(struct packet_info* pkt, struct node_info* node)
{
	int sig, noi, rate, bps, dps, bpsn, usen;
	int max = 0, avg = 0;
	float use;
	int max_stat_bar = stat_height - 4;

	werase(stat_win);
	wattron(stat_win, WHITE);
	mvwvline(stat_win, 0, 0, ACS_VLINE, stat_height);

	get_per_second(stats.bytes, stats.duration, &bps, &dps);

	bps *= 8;
	bpsn = normalize(bps, 32000000, max_stat_bar); //theoretical: 54000000

	use = dps * 1.0 / 10000; /* usec, in percent */
	usen = normalize(use, 100, max_stat_bar);

	if (pkt != NULL)
	{
		sig = normalize_db(-pkt->phy_signal, max_stat_bar);
		if (pkt->phy_noise)
			noi = normalize_db(-pkt->phy_noise, max_stat_bar);
		rate = normalize(pkt->phy_rate, 108, max_stat_bar);

		if (node != NULL && node->phy_sig_max < 0)
			max = normalize_db(-node->phy_sig_max, max_stat_bar);

		if (node != NULL)
			avg = normalize_db(-iir_average_get(node->phy_sig_avg), max_stat_bar);

		wattron(stat_win, GREEN);
		mvwprintw(stat_win, 0, 1, "SN:  %03d/", pkt->phy_signal);
		if (max > 1)
			mvwprintw(stat_win, max + 4, 2, "--");
		wattron(stat_win, ALLGREEN);
		mvwvline(stat_win, sig + 4, 2, ACS_BLOCK, stat_height - sig);
		mvwvline(stat_win, sig + 4, 3, ACS_BLOCK, stat_height - sig);

		if (avg > 1) {
			wattron(stat_win, A_BOLD);
			mvwvline(stat_win, avg + 4, 2, '=', stat_height - avg);
			wattron(stat_win, A_BOLD);
		}

		wattron(stat_win, RED);
		mvwprintw(stat_win, 0, 10, "%03d", pkt->phy_noise);

		if (pkt->phy_noise) {
			wattron(stat_win, ALLRED);
			mvwvline(stat_win, noi + 4, 2, '=', stat_height - noi);
			mvwvline(stat_win, noi + 4, 3, '=', stat_height - noi);
		}

		wattron(stat_win, BLUE);
		wattron(stat_win, A_BOLD);
		mvwprintw(stat_win, 1, 1, "PhyRate:  %2dM", pkt->phy_rate/2);
		wattroff(stat_win, A_BOLD);
		wattron(stat_win, ALLBLUE);
		mvwvline(stat_win, stat_height - rate, 5, ACS_BLOCK, rate);
		mvwvline(stat_win, stat_height - rate, 6, ACS_BLOCK, rate);
	}

	wattron(stat_win, CYAN);
	mvwprintw(stat_win, 2, 1, "b/sec: %6s", kilo_mega_ize(bps));
	wattron(stat_win, ALLCYAN);
	mvwvline(stat_win, stat_height - bpsn, 8, ACS_BLOCK, bpsn);
	mvwvline(stat_win, stat_height - bpsn, 9, ACS_BLOCK, bpsn);

	wattron(stat_win, YELLOW);
	mvwprintw(stat_win, 3, 1, "Usage: %5.1f%%", use);
	wattron(stat_win, ALLYELLOW);
	mvwvline(stat_win, stat_height - usen, 11, ACS_BLOCK, usen);
	mvwvline(stat_win, stat_height - usen, 12, ACS_BLOCK, usen);
	wattroff(stat_win, ALLYELLOW);

	wnoutrefresh(stat_win);
}


#define COL_IP		3
#define COL_SNR		COL_IP + 16
#define COL_RATE	COL_SNR + 9
#define COL_SOURCE	COL_RATE + 3
#define COL_STA		COL_SOURCE + 18
#define COL_BSSID	COL_STA + 2
#define COL_ENC		COL_BSSID + 20
#define COL_CHAN	COL_ENC + 2
#define COL_MESH	COL_CHAN + 3

static char spin[4] = {'/', '-', '\\', '|'};

static void
print_list_line(int line, struct node_info* n)
{
	struct packet_info* p = &n->last_pkt;

	if (n->pkt_types & PKT_TYPE_OLSR)
		wattron(list_win, GREEN);
	if (n->last_seen > (the_time.tv_sec - conf.node_timeout / 2))
		wattron(list_win, A_BOLD);
	else
		wattron(list_win, A_NORMAL);

	if (n->essid != NULL && n->essid->split > 0)
		wattron(list_win, RED);

	mvwprintw(list_win, line, 1, "%c", spin[n->pkt_count % 4]);

	mvwprintw(list_win, line, COL_SNR, "%2d/%2d/%2d",
		p->phy_snr, n->phy_snr_max, iir_average_get(n->phy_snr_avg));

	if (n->wlan_mode == WLAN_MODE_AP )
		mvwprintw(list_win, line, COL_STA,"A");
	else if (n->wlan_mode == WLAN_MODE_IBSS )
		mvwprintw(list_win, line, COL_STA, "I");
	else if (n->wlan_mode == WLAN_MODE_STA )
		mvwprintw(list_win, line, COL_STA, "S");
	else if (n->wlan_mode == WLAN_MODE_PROBE )
		mvwprintw(list_win, line, COL_STA, "P");

	mvwprintw(list_win, line, COL_ENC, n->wlan_wep ? "W" : "");

	mvwprintw(list_win, line, COL_RATE, "%2d", p->phy_rate/2);
	mvwprintw(list_win, line, COL_SOURCE, "%s", ether_sprintf(p->wlan_src));
	mvwprintw(list_win, line, COL_BSSID, "(%s)", ether_sprintf(n->wlan_bssid));

	if (n->wlan_channel)
		mvwprintw(list_win, line, COL_CHAN, "%2d", n->wlan_channel );

	if (n->pkt_types & PKT_TYPE_IP)
		mvwprintw(list_win, line, COL_IP, "%s", ip_sprintf(n->ip_src));

	if (n->pkt_types & PKT_TYPE_OLSR)
		mvwprintw(list_win, line, COL_MESH, "OLSR%s N:%d %s",
			n->pkt_types & PKT_TYPE_OLSR_LQ ? "_LQ" : "",
			n->olsr_neigh,
			n->pkt_types & PKT_TYPE_OLSR_GW ? "GW" : "");

	if (n->pkt_types & PKT_TYPE_BATMAN)
		wprintw(list_win, " BAT");

	wattroff(list_win, A_BOLD);
	wattroff(list_win, GREEN);
	wattroff(list_win, RED);
}


static void
update_list_win(void)
{
	struct node_info* n;
	int line = 0;

	werase(list_win);
	wattron(list_win, WHITE);
	box(list_win, 0 , 0);
	mvwprintw(list_win, 0, COL_SNR, "SN/MX/AV");
	mvwprintw(list_win, 0, COL_RATE, "RT");
	mvwprintw(list_win, 0, COL_SOURCE, "SOURCE");
	mvwprintw(list_win, 0, COL_STA, "M");
	mvwprintw(list_win, 0, COL_BSSID, "(BSSID)");
	mvwprintw(list_win, 0, COL_IP, "IP");
	mvwprintw(list_win, 0, COL_CHAN, "CH");
	mvwprintw(list_win, 0, COL_ENC, "E");
	mvwprintw(list_win, 0, COL_MESH, "Mesh");

	/* reuse bottom line for information on other win */
	mvwprintw(list_win, win_split - 1, 0, "Sig/Noi-RT-SOURCE");
	mvwprintw(list_win, win_split - 1, 29, "(BSSID)");
	mvwprintw(list_win, win_split - 1, 49, "TYPE");
	mvwprintw(list_win, win_split - 1, 56, "INFO");
	mvwprintw(list_win, win_split - 1, COLS-12, "LiveStatus");

	if (sortfunc)
		listsort(&nodes, sortfunc);

	list_for_each_entry(n, &nodes, list) {
		line++;
		if (line >= win_split - 1)
			break; /* prevent overdraw of last line */
		print_list_line(line, n);
	}

	if (essids.split_active > 0) {
		wattron(list_win, WHITEONRED);
		mvwhline(list_win, win_split - 2, 1, ' ', COLS - 2);
		print_centered(list_win, win_split - 2, COLS - 2,
			"*** IBSS SPLIT DETECTED!!! ESSID '%s' %d nodes ***",
			essids.split_essid->essid, essids.split_essid->num_nodes);
		wattroff(list_win, WHITEONRED);
	}

	wnoutrefresh(list_win);
}


void
update_dump_win(struct packet_info* pkt)
{
	if (!pkt) {
		redrawwin(dump_win);
		wnoutrefresh(dump_win);
		return;
	}

	wattron(dump_win, CYAN);

	if (pkt->olsr_type > 0 && pkt->pkt_types & PKT_TYPE_OLSR)
		wattron(dump_win, A_BOLD);

	wprintw(dump_win, "\n%03d/%03d ", pkt->phy_signal, pkt->phy_noise);
	wprintw(dump_win, "%2d ", pkt->phy_rate/2);
	wprintw(dump_win, "%s ", ether_sprintf(pkt->wlan_src));
	wprintw(dump_win, "(%s) ", ether_sprintf(pkt->wlan_bssid));

	if (pkt->wlan_retry)
		wprintw(dump_win, "[r]");

	if (pkt->pkt_types & PKT_TYPE_OLSR) {
		wprintw(dump_win, "%-7s%s ", "OLSR", ip_sprintf(pkt->ip_src));
		switch (pkt->olsr_type) {
			case HELLO_MESSAGE: wprintw(dump_win, "HELLO"); break;
			case TC_MESSAGE: wprintw(dump_win, "TC"); break;
			case MID_MESSAGE: wprintw(dump_win, "MID");break;
			case HNA_MESSAGE: wprintw(dump_win, "HNA"); break;
			case LQ_HELLO_MESSAGE: wprintw(dump_win, "LQ_HELLO"); break;
			case LQ_TC_MESSAGE: wprintw(dump_win, "LQ_TC"); break;
			default: wprintw(dump_win, "(%d)", pkt->olsr_type);
		}
	}
	else if (pkt->pkt_types & PKT_TYPE_UDP) {
		wprintw(dump_win, "%-7s%s", "UDP", ip_sprintf(pkt->ip_src));
		wprintw(dump_win, " -> %s", ip_sprintf(pkt->ip_dst));
	}
	else if (pkt->pkt_types & PKT_TYPE_TCP) {
		wprintw(dump_win, "%-7s%s", "TCP", ip_sprintf(pkt->ip_src));
		wprintw(dump_win, " -> %s", ip_sprintf(pkt->ip_dst));
	}
	else if (pkt->pkt_types & PKT_TYPE_ICMP) {
		wprintw(dump_win, "%-7s%s", "PING", ip_sprintf(pkt->ip_src));
		wprintw(dump_win, " -> %s", ip_sprintf(pkt->ip_dst));
	}
	else if (pkt->pkt_types & PKT_TYPE_IP) {
		wprintw(dump_win, "%-7s%s", "IP", ip_sprintf(pkt->ip_src));
		wprintw(dump_win, " -> %s", ip_sprintf(pkt->ip_dst));
	}
	else if (pkt->pkt_types & PKT_TYPE_ARP) {
		wprintw(dump_win, "%-7s", "ARP", ip_sprintf(pkt->ip_src));
	}
	else {
		wprintw(dump_win, "%-7s", get_packet_type_name(pkt->wlan_type));

		switch (pkt->wlan_type & IEEE80211_FCTL_FTYPE) {
		case IEEE80211_FTYPE_DATA:
			if ( pkt->wlan_wep == 1)
				wprintw(dump_win, "ENCRYPTED");
			break;
		case IEEE80211_FTYPE_CTL:
			switch (pkt->wlan_type & IEEE80211_FCTL_STYPE) {
			case IEEE80211_STYPE_CTS:
			case IEEE80211_STYPE_RTS:
			case IEEE80211_STYPE_ACK:
				wprintw(dump_win, "%s", ether_sprintf(pkt->wlan_dst));
				break;
			}
			break;
		case IEEE80211_FTYPE_MGMT:
			switch (pkt->wlan_type & IEEE80211_FCTL_STYPE) {
			case IEEE80211_STYPE_BEACON:
			case IEEE80211_STYPE_PROBE_RESP:
				wprintw(dump_win, "'%s' %llx", pkt->wlan_essid,
					pkt->wlan_tsf);
				break;
			case IEEE80211_STYPE_PROBE_REQ:
				wprintw(dump_win, "'%s'", pkt->wlan_essid);
				break;
			}
		}
	}
	wattroff(dump_win,A_BOLD);
}


void
print_dump_win(const char *str)
{
	wprintw(dump_win, str);
	wnoutrefresh(dump_win);
}


void
update_main_win(struct packet_info *pkt, struct node_info *node)
{
	update_list_win();
	update_status_win(pkt, node);
	update_dump_win(pkt);
	wnoutrefresh(dump_win);
	if (sort_win != NULL) {
		redrawwin(sort_win);
		wnoutrefresh(sort_win);
	}
}


void
main_input(char key)
{
	if (sort_win != NULL) {
		sort_input(key);
		return;
	}

	switch(key) {
	case 'o': case 'O':
		show_sort_win();
		break;
	}
}


void
init_display_main(void)
{
	win_split = LINES / 2 + 1;
	stat_height = LINES - win_split - 1;

	list_win = newwin(win_split, COLS, 0, 0);
	scrollok(list_win,FALSE);

	stat_win = newwin(stat_height, 14, win_split, COLS-14);
	scrollok(stat_win,FALSE);

	dump_win = newwin(stat_height, COLS-14, win_split, 0);
	scrollok(dump_win,TRUE);
}