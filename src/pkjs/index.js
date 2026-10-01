const KEYS = require("message_keys");

/**
 * @typedef {{route_type: number, stop_id: number, route_id: number, display_name: string}} Route
 * @typedef {{eta_secs: number, realtime: boolean, headsign: string}} Departure
 * @typedef {Record<number, number | string>} Message
 */

const KIND_SETTINGS = 0;
const KIND_STOPS = 1;
const KIND_ROUTES = 2;
const KIND_DEPARTURES = 3;

// Wire budget: sized keys cost 7 bytes of tuple overhead each, so names are the
// tuning knob. See the plan's budget check.
const NAME_MAX = 20;

const ROUTE_TYPE_TRAIN = 0;
const ROUTE_TYPE_TRAM = 1;
const ROUTE_TYPE_BUS = 2;
const ROUTE_TYPE_NIGHT_BUS = 4;

// Deterministic mock data, mirroring src/c/data.c so the emulator is
// reproducible and the offline fallback matches what the phone sends.

/** @type {{name: string, distance_m: number}[]} */
const STOPS = [
  { name: "Flinders St", distance_m: 120 },
  { name: "Southern Cross", distance_m: 240 },
  { name: "Melbourne Central", distance_m: 310 },
  { name: "Richmond", distance_m: 480 },
  { name: "Glenferrie", distance_m: 620 },
  { name: "Prahran", distance_m: 900 },
];

/** @type {number[][]} */
const ROUTE_TYPES = [
  [ROUTE_TYPE_TRAM, ROUTE_TYPE_BUS, ROUTE_TYPE_TRAIN],
  [ROUTE_TYPE_BUS, ROUTE_TYPE_NIGHT_BUS, ROUTE_TYPE_TRAM],
  [ROUTE_TYPE_TRAIN, ROUTE_TYPE_TRAM, ROUTE_TYPE_BUS],
];

/** @type {number[]} */
const ROUTE_NUMBERS = [109, 86, 48, 12, 75, 19];

/** @type {string[]} */
const HEADSIGNS = ["City", "South Cross", "Bundoora RMIT"];

/** @type {number[]} */
const ETA_SECS = [120, 420, 900, 1800];

/** @type {[number, number][]} [stop index, route slot] */
const FAVOURITE_SLOTS = [
  [0, 0],
  [1, 1],
  [2, 0],
  [3, 1],
];

/** @param {number} index */
const stopId = (index) => index + 1;

/** @param {number} id */
const stopIndexOf = (id) => id - 1;

/** @param {number} index @param {number} slot */
const routeId = (index, slot) => (index + 1) * 100 + slot + 1;

/** @param {number} index @param {number} slot */
const routeNumber = (index, slot) => ROUTE_NUMBERS[(index * 3 + slot) % 6];

/** @param {number} index @param {number} slot */
const headsign = (index, slot) => HEADSIGNS[(index + slot) % 3];

/** @param {number} index @param {number} slot @returns {Route} */
function routeAt(index, slot) {
  return {
    route_type: ROUTE_TYPES[index % 3][slot],
    stop_id: stopId(index),
    route_id: routeId(index, slot),
    display_name: `${routeNumber(index, slot)} ${headsign(index, slot)}`,
  };
}

/** @param {number} index @param {number} id */
function slotOfRoute(index, id) {
  for (let slot = 0; slot < 3; slot++) {
    if (routeId(index, slot) === id) {
      return slot;
    }
  }
  return -1;
}

/** @param {string} name */
function truncate(name) {
  return name.length > NAME_MAX ? name.slice(0, NAME_MAX) : name;
}

/**
 * A sized message key reserves N consecutive keys, so element i lives at
 * MESSAGE_KEY_base + i. There is no array type on the watch side.
 *
 * @template {Record<number, number | string>} T
 * @param {T} message
 * @param {number} baseKey
 * @param {(number | string)[]} values
 * @returns {T}
 */
function addIndexed(message, baseKey, values) {
  values.forEach((value, i) => {
    message[baseKey + i] = value;
  });
  return message;
}

/** @returns {Message} */
function settingsResponse() {
  /** @type {Route[]} */
  const favourites = FAVOURITE_SLOTS.map(([index, slot]) =>
    routeAt(index, slot)
  );

  /** @type {Message} */
  const message = {
    [KEYS.Kind]: KIND_SETTINGS,
    [KEYS.FavouriteCount]: favourites.length,
  };

  addIndexed(
    message,
    KEYS.FavouriteRouteTypes,
    favourites.map((f) => f.route_type)
  );
  addIndexed(
    message,
    KEYS.FavouriteStopIDs,
    favourites.map((f) => f.stop_id)
  );
  addIndexed(
    message,
    KEYS.FavouriteRouteIDs,
    favourites.map((f) => f.route_id)
  );
  addIndexed(
    message,
    KEYS.FavouriteDisplayNames,
    favourites.map((f) => truncate(f.display_name))
  );

  return message;
}

/** @returns {Message} */
function stopsResponse() {
  /** @type {Message} */
  const message = {
    [KEYS.Kind]: KIND_STOPS,
    [KEYS.StopsCount]: STOPS.length,
  };

  addIndexed(
    message,
    KEYS.StopIDs,
    STOPS.map((_, i) => stopId(i))
  );
  addIndexed(
    message,
    KEYS.StopDistances,
    STOPS.map((s) => s.distance_m)
  );
  addIndexed(
    message,
    KEYS.StopNames,
    STOPS.map((s) => truncate(s.name))
  );

  return message;
}

/** @param {number} stop @returns {Message | null} */
function routesResponse(stop) {
  const index = stopIndexOf(stop);
  if (index < 0 || index >= STOPS.length) {
    return null;
  }

  /** @type {Route[]} */
  const routes = [0, 1, 2].map((slot) => routeAt(index, slot));

  /** @type {Message} */
  const message = {
    [KEYS.Kind]: KIND_ROUTES,
    // StopID is echoed so the watch can attribute the response; there is no
    // correlation id in this protocol.
    [KEYS.StopID]: stop,
    [KEYS.RoutesCount]: routes.length,
  };

  addIndexed(
    message,
    KEYS.RouteTypes,
    routes.map((r) => r.route_type)
  );
  addIndexed(
    message,
    KEYS.RouteIDs,
    routes.map((r) => r.route_id)
  );
  addIndexed(
    message,
    KEYS.RouteNames,
    routes.map((r) => truncate(r.display_name))
  );

  return message;
}

/** @param {number} stop @param {number} route @returns {Message | null} */
function departuresResponse(stop, route) {
  const index = stopIndexOf(stop);
  const slot = slotOfRoute(index, route);
  if (slot < 0) {
    return null;
  }

  /** @type {Departure[]} */
  const departures = ETA_SECS.map((eta, i) => ({
    eta_secs: eta,
    realtime: i < 2,
    headsign: headsign(index, slot),
  }));

  /** @type {Message} */
  const message = {
    [KEYS.Kind]: KIND_DEPARTURES,
    [KEYS.StopID]: stop,
    [KEYS.RouteID]: route,
    [KEYS.DeparturesCount]: departures.length,
  };

  addIndexed(
    message,
    KEYS.DepartureETAs,
    departures.map((d) => d.eta_secs)
  );
  addIndexed(
    message,
    KEYS.DepartureFlags,
    departures.map((d) => (d.realtime ? 1 : 0))
  );
  addIndexed(
    message,
    KEYS.DepartureHeadsigns,
    departures.map((d) => truncate(d.headsign))
  );

  return message;
}

/** @param {{data: Message}} event */
function onAppMessage(event) {
  console.log("RAW " + JSON.stringify(event));
  const request = event.data || event;
  /** @type {Message | null} */
  let response;

  switch (request[KEYS.Kind]) {
    case KIND_SETTINGS:
      response = settingsResponse();
      break;
    case KIND_STOPS:
      response = stopsResponse();
      break;
    case KIND_ROUTES:
      response = routesResponse(Number(request[KEYS.StopID]));
      break;
    case KIND_DEPARTURES:
      response = departuresResponse(
        Number(request[KEYS.StopID]),
        Number(request[KEYS.RouteID])
      );
      break;
    default:
      return;
  }

  if (response) {
    Pebble.sendAppMessage(response);
  }
}

Pebble.addEventListener("ready", () => {
  Pebble.addEventListener("appmessage", onAppMessage);
  console.log("ptvbuddy ready");
});
