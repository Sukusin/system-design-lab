const dbName = 'hotel_booking_mongo';
db = db.getSiblingDB(dbName);

const userIds = Array.from({ length: 10 }, (_, i) => ObjectId(`65f0000000000000000000${(i + 1).toString(16).padStart(2, '0')}`));
const hotelIds = Array.from({ length: 10 }, (_, i) => ObjectId(`65f0000000000000000001${(i + 1).toString(16).padStart(2, '0')}`));
const bookingIds = Array.from({ length: 12 }, (_, i) => ObjectId(`65f0000000000000000002${(i + 1).toString(16).padStart(2, '0')}`));

const users = [
  { _id: userIds[0], login: 'kirill_sokolov', password_hash: 'seed$hash_01_long_value', first_name: 'Kirill', last_name: 'Sokolov', created_at: ISODate('2026-04-01T09:00:00Z') },
  { _id: userIds[1], login: 'anna_ivanova', password_hash: 'seed$hash_02_long_value', first_name: 'Anna', last_name: 'Ivanova', created_at: ISODate('2026-04-01T09:15:00Z') },
  { _id: userIds[2], login: 'oleg_petrov', password_hash: 'seed$hash_03_long_value', first_name: 'Oleg', last_name: 'Petrov', created_at: ISODate('2026-04-01T09:30:00Z') },
  { _id: userIds[3], login: 'maria_smile', password_hash: 'seed$hash_04_long_value', first_name: 'Maria', last_name: 'Smirnova', created_at: ISODate('2026-04-01T09:45:00Z') },
  { _id: userIds[4], login: 'denis_volkov', password_hash: 'seed$hash_05_long_value', first_name: 'Denis', last_name: 'Volkov', created_at: ISODate('2026-04-01T10:00:00Z') },
  { _id: userIds[5], login: 'elena_orlova', password_hash: 'seed$hash_06_long_value', first_name: 'Elena', last_name: 'Orlova', created_at: ISODate('2026-04-01T10:15:00Z') },
  { _id: userIds[6], login: 'roman_fedorov', password_hash: 'seed$hash_07_long_value', first_name: 'Roman', last_name: 'Fedorov', created_at: ISODate('2026-04-01T10:30:00Z') },
  { _id: userIds[7], login: 'irina_lebedeva', password_hash: 'seed$hash_08_long_value', first_name: 'Irina', last_name: 'Lebedeva', created_at: ISODate('2026-04-01T10:45:00Z') },
  { _id: userIds[8], login: 'pavel_kozlov', password_hash: 'seed$hash_09_long_value', first_name: 'Pavel', last_name: 'Kozlov', created_at: ISODate('2026-04-01T11:00:00Z') },
  { _id: userIds[9], login: 'sofia_nikolaeva', password_hash: 'seed$hash_10_long_value', first_name: 'Sofia', last_name: 'Nikolaeva', created_at: ISODate('2026-04-01T11:15:00Z') }
];

const hotels = [
  { _id: hotelIds[0], name: 'Red Square Hotel', city: 'Moscow', address: 'Tverskaya 12', stars: NumberInt(4), rooms_total: NumberInt(28), amenities: ['wifi', 'breakfast', 'parking'], created_by: userIds[0], created_at: ISODate('2026-04-02T08:00:00Z') },
  { _id: hotelIds[1], name: 'Garden Ring Stay', city: 'Moscow', address: 'Arbat 18', stars: NumberInt(3), rooms_total: NumberInt(18), amenities: ['wifi', 'bike-rental'], created_by: userIds[1], created_at: ISODate('2026-04-02T08:20:00Z') },
  { _id: hotelIds[2], name: 'Neva Palace Hotel', city: 'Saint Petersburg', address: 'Nevsky 45', stars: NumberInt(5), rooms_total: NumberInt(35), amenities: ['wifi', 'spa', 'parking'], created_by: userIds[2], created_at: ISODate('2026-04-02T08:40:00Z') },
  { _id: hotelIds[3], name: 'Baltic Port Hotel', city: 'Saint Petersburg', address: 'Ligovsky 27', stars: NumberInt(4), rooms_total: NumberInt(24), amenities: ['wifi', 'gym'], created_by: userIds[3], created_at: ISODate('2026-04-02T09:00:00Z') },
  { _id: hotelIds[4], name: 'Volga Star Hotel', city: 'Kazan', address: 'Baumana 9', stars: NumberInt(5), rooms_total: NumberInt(30), amenities: ['wifi', 'breakfast', 'conference'], created_by: userIds[4], created_at: ISODate('2026-04-02T09:20:00Z') },
  { _id: hotelIds[5], name: 'Kremlin View Inn', city: 'Kazan', address: 'Universitetskaya 14', stars: NumberInt(4), rooms_total: NumberInt(21), amenities: ['wifi', 'sea-view'], created_by: userIds[5], created_at: ISODate('2026-04-02T09:40:00Z') },
  { _id: hotelIds[6], name: 'Black Sea Resort', city: 'Sochi', address: 'Kurortny 21', stars: NumberInt(4), rooms_total: NumberInt(22), amenities: ['wifi', 'breakfast', 'parking'], created_by: userIds[6], created_at: ISODate('2026-04-02T10:00:00Z') },
  { _id: hotelIds[7], name: 'Mountain Breeze Hotel', city: 'Sochi', address: 'Morskaya 8', stars: NumberInt(2), rooms_total: NumberInt(40), amenities: ['wifi', 'laundry'], created_by: userIds[7], created_at: ISODate('2026-04-02T10:20:00Z') },
  { _id: hotelIds[8], name: 'Siberia Central Hotel', city: 'Novosibirsk', address: 'Lenina 33', stars: NumberInt(3), rooms_total: NumberInt(19), amenities: ['wifi', 'breakfast'], created_by: userIds[8], created_at: ISODate('2026-04-02T10:40:00Z') },
  { _id: hotelIds[9], name: 'Ob River Suites', city: 'Novosibirsk', address: 'Sovetskaya 17', stars: NumberInt(5), rooms_total: NumberInt(26), amenities: ['wifi', 'gym', 'parking'], created_by: userIds[9], created_at: ISODate('2026-04-02T11:00:00Z') }
];

const bookings = [
  {
    _id: bookingIds[0], user_id: userIds[0], hotel_id: hotelIds[0],
    hotel_snapshot: { name: 'Red Square Hotel', city: 'Moscow', address: 'Tverskaya 12', stars: NumberInt(4) },
    stay: { check_in: ISODate('2026-05-10T00:00:00Z'), check_out: ISODate('2026-05-15T00:00:00Z'), guests: NumberInt(2) },
    status: 'active', created_at: ISODate('2026-04-03T08:00:00Z'), cancelled_at: null,
    status_history: [{ status: 'active', changed_at: ISODate('2026-04-03T08:00:00Z'), comment: 'booking created' }]
  },
  {
    _id: bookingIds[1], user_id: userIds[1], hotel_id: hotelIds[1],
    hotel_snapshot: { name: 'Garden Ring Stay', city: 'Moscow', address: 'Arbat 18', stars: NumberInt(3) },
    stay: { check_in: ISODate('2026-05-12T00:00:00Z'), check_out: ISODate('2026-05-16T00:00:00Z'), guests: NumberInt(1) },
    status: 'active', created_at: ISODate('2026-04-03T09:00:00Z'), cancelled_at: null,
    status_history: [{ status: 'active', changed_at: ISODate('2026-04-03T09:00:00Z'), comment: 'booking created' }]
  },
  {
    _id: bookingIds[2], user_id: userIds[2], hotel_id: hotelIds[2],
    hotel_snapshot: { name: 'Neva Palace Hotel', city: 'Saint Petersburg', address: 'Nevsky 45', stars: NumberInt(5) },
    stay: { check_in: ISODate('2026-05-18T00:00:00Z'), check_out: ISODate('2026-05-22T00:00:00Z'), guests: NumberInt(2) },
    status: 'active', created_at: ISODate('2026-04-03T10:00:00Z'), cancelled_at: null,
    status_history: [{ status: 'active', changed_at: ISODate('2026-04-03T10:00:00Z'), comment: 'booking created' }]
  },
  {
    _id: bookingIds[3], user_id: userIds[3], hotel_id: hotelIds[3],
    hotel_snapshot: { name: 'Baltic Port Hotel', city: 'Saint Petersburg', address: 'Ligovsky 27', stars: NumberInt(4) },
    stay: { check_in: ISODate('2026-05-20T00:00:00Z'), check_out: ISODate('2026-05-23T00:00:00Z'), guests: NumberInt(3) },
    status: 'active', created_at: ISODate('2026-04-03T11:00:00Z'), cancelled_at: null,
    status_history: [{ status: 'active', changed_at: ISODate('2026-04-03T11:00:00Z'), comment: 'booking created' }]
  },
  {
    _id: bookingIds[4], user_id: userIds[4], hotel_id: hotelIds[4],
    hotel_snapshot: { name: 'Volga Star Hotel', city: 'Kazan', address: 'Baumana 9', stars: NumberInt(5) },
    stay: { check_in: ISODate('2026-05-21T00:00:00Z'), check_out: ISODate('2026-05-27T00:00:00Z'), guests: NumberInt(2) },
    status: 'active', created_at: ISODate('2026-04-03T12:00:00Z'), cancelled_at: null,
    status_history: [{ status: 'active', changed_at: ISODate('2026-04-03T12:00:00Z'), comment: 'booking created' }]
  },
  {
    _id: bookingIds[5], user_id: userIds[5], hotel_id: hotelIds[5],
    hotel_snapshot: { name: 'Kremlin View Inn', city: 'Kazan', address: 'Universitetskaya 14', stars: NumberInt(4) },
    stay: { check_in: ISODate('2026-05-25T00:00:00Z'), check_out: ISODate('2026-05-30T00:00:00Z'), guests: NumberInt(2) },
    status: 'cancelled', created_at: ISODate('2026-04-03T13:00:00Z'), cancelled_at: ISODate('2026-04-10T08:30:00Z'),
    status_history: [
      { status: 'active', changed_at: ISODate('2026-04-03T13:00:00Z'), comment: 'booking created' },
      { status: 'cancelled', changed_at: ISODate('2026-04-10T08:30:00Z'), comment: 'cancelled by user' }
    ]
  },
  {
    _id: bookingIds[6], user_id: userIds[6], hotel_id: hotelIds[6],
    hotel_snapshot: { name: 'Black Sea Resort', city: 'Sochi', address: 'Kurortny 21', stars: NumberInt(4) },
    stay: { check_in: ISODate('2026-06-01T00:00:00Z'), check_out: ISODate('2026-06-04T00:00:00Z'), guests: NumberInt(1) },
    status: 'active', created_at: ISODate('2026-04-03T14:00:00Z'), cancelled_at: null,
    status_history: [{ status: 'active', changed_at: ISODate('2026-04-03T14:00:00Z'), comment: 'booking created' }]
  },
  {
    _id: bookingIds[7], user_id: userIds[7], hotel_id: hotelIds[7],
    hotel_snapshot: { name: 'Mountain Breeze Hotel', city: 'Sochi', address: 'Morskaya 8', stars: NumberInt(2) },
    stay: { check_in: ISODate('2026-06-05T00:00:00Z'), check_out: ISODate('2026-06-10T00:00:00Z'), guests: NumberInt(1) },
    status: 'active', created_at: ISODate('2026-04-03T15:00:00Z'), cancelled_at: null,
    status_history: [{ status: 'active', changed_at: ISODate('2026-04-03T15:00:00Z'), comment: 'booking created' }]
  },
  {
    _id: bookingIds[8], user_id: userIds[8], hotel_id: hotelIds[8],
    hotel_snapshot: { name: 'Siberia Central Hotel', city: 'Novosibirsk', address: 'Lenina 33', stars: NumberInt(3) },
    stay: { check_in: ISODate('2026-06-11T00:00:00Z'), check_out: ISODate('2026-06-13T00:00:00Z'), guests: NumberInt(2) },
    status: 'cancelled', created_at: ISODate('2026-04-03T16:00:00Z'), cancelled_at: ISODate('2026-04-14T17:10:00Z'),
    status_history: [
      { status: 'active', changed_at: ISODate('2026-04-03T16:00:00Z'), comment: 'booking created' },
      { status: 'cancelled', changed_at: ISODate('2026-04-14T17:10:00Z'), comment: 'cancelled by user' }
    ]
  },
  {
    _id: bookingIds[9], user_id: userIds[9], hotel_id: hotelIds[9],
    hotel_snapshot: { name: 'Ob River Suites', city: 'Novosibirsk', address: 'Sovetskaya 17', stars: NumberInt(5) },
    stay: { check_in: ISODate('2026-06-15T00:00:00Z'), check_out: ISODate('2026-06-20T00:00:00Z'), guests: NumberInt(2) },
    status: 'active', created_at: ISODate('2026-04-03T17:00:00Z'), cancelled_at: null,
    status_history: [{ status: 'active', changed_at: ISODate('2026-04-03T17:00:00Z'), comment: 'booking created' }]
  },
  {
    _id: bookingIds[10], user_id: userIds[0], hotel_id: hotelIds[2],
    hotel_snapshot: { name: 'Neva Palace Hotel', city: 'Saint Petersburg', address: 'Nevsky 45', stars: NumberInt(5) },
    stay: { check_in: ISODate('2026-07-02T00:00:00Z'), check_out: ISODate('2026-07-06T00:00:00Z'), guests: NumberInt(2) },
    status: 'active', created_at: ISODate('2026-04-04T08:00:00Z'), cancelled_at: null,
    status_history: [{ status: 'active', changed_at: ISODate('2026-04-04T08:00:00Z'), comment: 'booking created' }]
  },
  {
    _id: bookingIds[11], user_id: userIds[1], hotel_id: hotelIds[4],
    hotel_snapshot: { name: 'Volga Star Hotel', city: 'Kazan', address: 'Baumana 9', stars: NumberInt(5) },
    stay: { check_in: ISODate('2026-07-10T00:00:00Z'), check_out: ISODate('2026-07-14T00:00:00Z'), guests: NumberInt(1) },
    status: 'active', created_at: ISODate('2026-04-04T09:00:00Z'), cancelled_at: null,
    status_history: [{ status: 'active', changed_at: ISODate('2026-04-04T09:00:00Z'), comment: 'booking created' }]
  }
];

db.bookings.deleteMany({});
db.hotels.deleteMany({});
db.users.deleteMany({});

db.users.insertMany(users);
db.hotels.insertMany(hotels);
db.bookings.insertMany(bookings);

print('Inserted users:', db.users.countDocuments());
print('Inserted hotels:', db.hotels.countDocuments());
print('Inserted bookings:', db.bookings.countDocuments());
