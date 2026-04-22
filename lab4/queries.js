// Открыть shell:
// docker exec -it hotel-booking-mongo mongosh -u root -p root --authenticationDatabase admin
// use hotel_booking_mongo

const dbName = 'hotel_booking_mongo';
db = db.getSiblingDB(dbName);

// =====================================================
// CREATE
// =====================================================

// 1. Создание нового пользователя
db.users.insertOne({
  login: 'new_user_01',
  password_hash: 'manual$hash_value_for_demo',
  first_name: 'Nikita',
  last_name: 'Romanov',
  created_at: new Date()
});

// 2. Создание нового отеля
const creatorId = db.users.findOne({ login: 'kirill_sokolov' })._id;
const hotelInsertResult = db.hotels.insertOne({
  name: 'Golden Ring Hotel',
  city: 'Moscow',
  address: 'Polevaya 5',
  stars: NumberInt(4),
  rooms_total: NumberInt(20),
  amenities: ['wifi', 'breakfast'],
  created_by: creatorId,
  created_at: new Date()
});

// 3. Создание нового бронирования
const userId = db.users.findOne({ login: 'anna_ivanova' })._id;
const hotel = db.hotels.findOne({ name: 'Red Square Hotel' });
db.bookings.insertOne({
  user_id: userId,
  hotel_id: hotel._id,
  hotel_snapshot: {
    name: hotel.name,
    city: hotel.city,
    address: hotel.address,
    stars: hotel.stars
  },
  stay: {
    check_in: ISODate('2026-08-01T00:00:00Z'),
    check_out: ISODate('2026-08-05T00:00:00Z'),
    guests: NumberInt(2)
  },
  status: 'active',
  created_at: new Date(),
  cancelled_at: null,
  status_history: [
    { status: 'active', changed_at: new Date(), comment: 'booking created manually' }
  ]
});

// =====================================================
// READ
// =====================================================

// 4. Поиск пользователя по логину ($eq)
db.users.findOne({ login: { $eq: 'kirill_sokolov' } });

// 5. Поиск пользователя по маске имени и фамилии ($or)
db.users.find({
  $or: [
    { first_name: /ir/i },
    { last_name: /ir/i }
  ]
});

// 6. Получение списка отелей
db.hotels.find().sort({ city: 1, name: 1 });

// 7. Поиск отелей по городу ($eq)
db.hotels.find({ city: { $eq: 'Moscow' } }).sort({ name: 1 });

// 8. Поиск отелей со сложным условием ($and, $gt, $lt, $in)
db.hotels.find({
  $and: [
    { stars: { $gt: 3 } },
    { rooms_total: { $lt: 30 } },
    { amenities: { $in: ['parking'] } }
  ]
});

// 9. Получение бронирований пользователя ($ne)
const kirillId = db.users.findOne({ login: 'kirill_sokolov' })._id;
db.bookings.find({
  user_id: kirillId,
  status: { $ne: 'cancelled' }
}).sort({ created_at: -1 });

// 10. Проверка пересечения дат для выбранного отеля
const checkIn = ISODate('2026-05-10T00:00:00Z');
const checkOut = ISODate('2026-05-15T00:00:00Z');
db.bookings.find({
  hotel_id: db.hotels.findOne({ name: 'Red Square Hotel' })._id,
  status: 'active',
  'stay.check_in': { $lt: checkOut },
  'stay.check_out': { $gt: checkIn }
});

// =====================================================
// UPDATE
// =====================================================

// 11. Добавить удобство отелю ($addToSet)
db.hotels.updateOne(
  { name: 'Red Square Hotel' },
  { $addToSet: { amenities: 'spa' } }
);

// 12. Добавить временный тег в массив ($push)
db.hotels.updateOne(
  { name: 'Red Square Hotel' },
  { $push: { amenities: 'late-checkout' } }
);

// 13. Удалить ненужное удобство ($pull)
db.hotels.updateOne(
  { name: 'Red Square Hotel' },
  { $pull: { amenities: 'late-checkout' } }
);

// 14. Отмена бронирования
const bookingToCancel = db.bookings.findOne({ status: 'active' });
db.bookings.updateOne(
  { _id: bookingToCancel._id, status: 'active' },
  {
    $set: {
      status: 'cancelled',
      cancelled_at: new Date()
    },
    $push: {
      status_history: {
        status: 'cancelled',
        changed_at: new Date(),
        comment: 'cancelled from mongosh'
      }
    }
  }
);

// =====================================================
// DELETE
// =====================================================

// 15. Удаление тестового пользователя
// Лучше выполнять только для специально созданных записей, которые не участвуют в бронированиях.
db.users.deleteOne({ login: 'new_user_01' });

// 16. Удаление отменённого тестового бронирования
db.bookings.deleteOne({
  status: 'cancelled',
  'status_history.comment': 'cancelled from mongosh'
});

// =====================================================
// AGGREGATION
// =====================================================

// 17. Сколько активных бронирований в каждом городе
// Используются стадии $match, $group, $project, $sort.
db.bookings.aggregate([
  { $match: { status: 'active' } },
  {
    $group: {
      _id: '$hotel_snapshot.city',
      active_bookings: { $sum: 1 },
      avg_guests: { $avg: '$stay.guests' }
    }
  },
  {
    $project: {
      _id: 0,
      city: '$_id',
      active_bookings: 1,
      avg_guests: { $round: ['$avg_guests', 2] }
    }
  },
  { $sort: { active_bookings: -1, city: 1 } }
]);
