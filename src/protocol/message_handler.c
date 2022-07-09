#include "src/protocol/message_handler.h"
#include "src/protocol/message.h"
#include "src/protocol/routing.h"
#include "src/protocol/search.h"
#include "src/transport.h"
#include <assert.h>
#include <stdio.h>

typedef bool (*handler_f)(message_t *msg);

bool handle_do_mute(message_t *msg);
bool handle_dont_mute(message_t *msg);
bool handle_do_update(message_t *msg);
bool handle_do_swap(message_t *msg);
bool handle_will_swap(message_t *msg);
bool handle_wont_swap(message_t *msg);
bool handle_do_report(message_t *msg);
bool handle_will_report(message_t *msg);
bool handle_do_transfer(message_t *msg);
bool handle_will_transfer(message_t *msg);
bool handle_do_find(message_t *msg);
bool handle_will_find(message_t *msg);

typedef enum {
  LEADER = 1 << 0,
  TREE_SWAPPING = 1 << 1,
  MUTED = 1 << 2,
  TRANSFERING = 1 << 3,
  SEARCHING = 1 << 4,
} flags_t;

flags_t global_flags = {0};

static handler_f message_handlers[MESSAGE_ACTION_COUNT][MESSAGE_TYPE_COUNT] = {
    [DO][MUTE] = handle_do_mute,
    [DONT][MUTE] = handle_dont_mute,
    [DO][UPDATE] = handle_do_update,
    [DO][SWAP] = handle_do_swap,
    [WILL][SWAP] = handle_will_swap,
    [WONT][SWAP] = handle_wont_swap,
    [DO][REPORT] = handle_do_report,
    [WILL][REPORT] = handle_will_report,
    [DO][TRANSFER] = handle_do_transfer,
    [WILL][TRANSFER] = handle_will_transfer,
    [DO][FIND] = handle_do_find,
    [WILL][FIND] = handle_will_find};


int message_cmp(message_t a, message_t b);

MAKE_SPECIFIC_PRIORITY_QUEUE_HEADER(message_t, message)
MAKE_SPECIFIC_VECTOR_SOURCE(message_t, message);
MAKE_SPECIFIC_PRIORITY_QUEUE_SOURCE(message_t, message, message_cmp)

static message_priority_queue_t *to_send;

bool handle_do_mute(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == MUTE);

  if (!message_from_leader(msg)) {
    fprintf(stderr, "Received DO MUTE from non-leader, will not act on it.\n");
    return false;
  }

  global_flags |= MUTED;

  return true;
}

bool handle_dont_mute(message_t *msg) {
  assert(message_action(msg) == DONT);
  assert(message_type(msg) == MUTE);

  if (!message_from_leader(msg)) {
    fprintf(stderr,
            "Received DONT MUTE from non-leader, will not act on it.\n");
    return false;
  }

  global_flags &= ~MUTED;

  return true;
}

// FIXME: muss Möglichkeit geben, Antwort als zweiten Parameter aus Funktion zu
// geben.
bool handle_do_update(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == UPDATE);

  // TODO:
  // WILL UPDATE aus Funktion zurückgeben.
  // überlegen, ob man in Antwort noch weitere Informationen zurückgeben
  // könnte oder ob ein einfaches "Pong" ausreicht.

  return true;
}

// FIXME: braucht globales struct, in dem parent, lhs und rhs stehen.
// FIXME: braucht vllt Variable, in der steht, an welche Frequenz Swapping
// angefragt wurde
bool handle_do_swap(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == SWAP);

  // FIXME: hier müssen irgendwo noch alle nonleader gemutet werden. Macht das
  // der Antragssteller, nachdem er auf Frequenz wechselt oder macht man das
  // selbst?
  // TODO:
  // 1) prüfen ob Nachricht von Leader
  // 2) prüfen ob Frequenz lhs oder rhs ist (DO SWAP sollte nur an im Baum höher
  // liegende Frequenzen gesendet werden)
  // 2.1) wenn nein => WONT SWAP als Antwort
  // 3) Activity Score für eigene aktuelle Frequenz berechnen (kann
  // vielleicht auch fortlaufend in anderen Handlern gemacht werden?)
  // 4) eigenen Activity Score mit gesendetem Activity Score vergleichen
  // 4.1) wenn kleiner =>
  //      TREE_SWAPPING Flag setzen (braucht vllt Timeout falls Antragsteller
  //      nicht mehr antwortet)
  //      WILL SWAP als Antwort
  //      zu eigener parent Frequenz wechseln, DO UPDATE an Leader schicken
  //      zu eigener lhs oder rhs Frequenz wechseln, je nachdem wer
  //      Antragssteller ist, DO UPDATE an Leader schicken
  //      eigene parent, lhs und rhs anpassen
  //      TREE_SWAPPING Flag clearen
  // 4.2) wenn größer => WONT SWAP als Antwort

  return true;
}

bool handle_will_swap(message_t *msg) {
  assert(message_action(msg) == WILL);
  assert(message_type(msg) == SWAP);

  // TODO:
  // 1) prüfen ob TREE_SWAPPING Flag gesetzt ist
  // 1) prüfen ob Nachricht von Leader
  // 2) prüfen ob Frequenz parent ist
  // 3) zu eigener lhs und rhs Frequenz wechseln, DO UPDATE an Leader schicken
  // 4) eigene parent, lhs und rhs anpassen
  // 5) TREE_SWAPPING Flag clearen

  return true;
}

bool handle_wont_swap(message_t *msg) {
  assert(message_action(msg) == WONT);
  assert(message_type(msg) == SWAP);

  return true;
}

// FIXME: braucht globale Report Timestamp
// wenn es normalerweise alle N Zeitschritte ein DO REPORT gibt, kann man
// eigentlich davon ausgehen, dass der Leader nach 1.5 * N + random()
// Zeitschritten inaktiv ist. In dem Fall sollte ein neuer Leader gewählt
// werden. Sobald bei einem Node dieser Fall eintritt, sollte er die folgenden
// Schritte ausführen:
// 1) LEADER Flag setzen
// 2) DO REPORT an alle verschicken
// 3) Next Scheduled Report Timestamp auf aktuelle Timestamp + N setzen
//
// FIXME: Was ist wenn sich mehrere Nodes gleichzeitig zum Leader ernennen?
// Um das Problem zu lösen, kann 1.5 * N + random() mit dem DO REPORT Paket
// verschickt werden. Wer den kleinsten Wert hat, wird zum Leader (wir gehen
// hier mal davon aus, dass die verschwindend kleine Chance, dass die beiden
// Werte gleich sind, nicht vorkommt). Das ist auch cool, weil dann variabel
// gesteuert werden kann, wie oft ein REPORT verschickt wird.
// Das würde ungefähr so ablaufen:
// 1) Leader verschickt Report mit N = 10000, setzt Next Interval auf N = 10000
// 2) Nodes setzen Next Interval auf 1.5 * N + random() = 15000 + random()
// 3) Leader verschickt danach Report mit N = 5000
// 4) Nodes setzen Next Interval auf 1.5 * N + random() = 7500 + random()
// Der Leader kann so das Report Interval selbst festlegen, ohne Gefahr zu
// laufen, dass er deswegen gestürzt wird ;)
bool handle_do_report(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == REPORT);

  // TODO:
  // 1) Next Interval zurücksetzen (aktuelle Zeit + 1.5 * N +
  // random())
  // 2) Mit WILL REPORT antworten

  return true;
}

bool handle_will_report(message_t *msg) {
  assert(message_action(msg) == WILL);
  assert(message_type(msg) == REPORT);

  return true;
}

bool handle_do_transfer(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == TRANSFER);
  
  routing_id_t sender = msg->header.sender_id;
  assert(sender.layer == leader);

  frequency_t destination_frequency = msg->payload.transfer.to;

  change_frequency(destination_frequency);
  // TODO go into state where newly joined a frequency

  return true;
}

bool handle_will_transfer(message_t *msg) {
  assert(message_action(msg) == WILL);
  assert(message_type(msg) == TRANSFER);

  return true;
}

inline bool set_sender_id_layer(routing_id_t* sender_id){
  if(global_flags & LEADER){
    sender_id->layer = leader;
  } else {
    sender_id->layer = nonleader;
  }

  return true;
}

inline message_header_t will_find_assemble_header(routing_id_t sender_id, routing_id_t receiver_id){
  return (message_header_t){WILL, FIND, sender_id, receiver_id};
}

bool handle_do_find(message_t *msg) {
  assert(message_action(msg) == DO);
  assert(message_type(msg) == FIND);

  routing_id_t to_find = msg->payload.find.to_find;
  routing_id_t self_id;
  get_id(self_id.optional_MAC);
  bool searching_for_self = routing_id_MAC_equal(to_find, self_id);

  // TODO possible case: node is looking for me but I am not registered
  if(!leader && !searching_for_self){
    return false;
  }

  message_t reply_msg;
  set_sender_id_layer(&self_id);
  reply_msg.header = will_find_assemble_header(self_id, msg->header.sender_id);

  // leader informs where node might be found
  if(!searching_for_self){
    //TODO
    /*reply_msg.payload.find.frequencies.lhs = lhs;*/
    /*reply_msg.payload.find.frequencies.rhs = rhs;*/
  }

  message_priority_queue_push(to_send, reply_msg);

  return true;
}

bool handle_will_find(message_t *msg) {
  assert(message_action(msg) == WILL);
  assert(message_type(msg) == FIND);
  assert(global_flags & SEARCHING);

  routing_id_t sender = msg->header.sender_id;

  if(routing_id_MAC_equal(sender, get_to_find())){
    search_concluded();
    // TODO register in frequency
    return true;
  } else {
    assert(sender.layer == leader);
    search_frequencies_queue_add(msg->payload.find.frequencies.lhs);
    search_frequencies_queue_add(msg->payload.find.frequencies.rhs);
  }

  return true;
}

bool handle_message(message_t *msg) {
  assert(message_is_valid(msg));

  return message_handlers[message_action(msg)][message_type(msg)](msg);
}
