FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest AS build
WORKDIR /app
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j

FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest
WORKDIR /service
COPY --from=build /app/build/hotel_booking_mongo_userver /service/hotel_booking_mongo_userver
COPY static_config.yaml /service/static_config.yaml
EXPOSE 8080
CMD ["/service/hotel_booking_mongo_userver", "-c", "/service/static_config.yaml"]
