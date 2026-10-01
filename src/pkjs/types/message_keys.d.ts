// Sized keys such as `FavouriteStopIDs[8]` reserve 8 consecutive keys; only the
// base name is declared here. Element i is `MESSAGE_KEY_base + i`.
declare module "message_keys" {
  const keys: {
    readonly Kind: number;
    readonly FavouriteCount: number;
    readonly FavouriteRouteTypes: number;
    readonly FavouriteStopIDs: number;
    readonly FavouriteRouteIDs: number;
    readonly FavouriteDisplayNames: number;
    readonly StopsCount: number;
    readonly StopIDs: number;
    readonly StopDistances: number;
    readonly StopNames: number;
    readonly RoutesCount: number;
    readonly RouteTypes: number;
    readonly RouteIDs: number;
    readonly RouteNames: number;
    readonly DeparturesCount: number;
    readonly DepartureETAs: number;
    readonly DepartureFlags: number;
    readonly DepartureHeadsigns: number;
    readonly StopID: number;
    readonly RouteID: number;
  };
  export = keys;
}
