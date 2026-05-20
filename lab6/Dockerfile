FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest AS build
WORKDIR /app
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j

FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest
WORKDIR /service
COPY --from=build /app/build/hotel_booking_hw06_api_userver /service/hotel_booking_hw06_api_userver
COPY --from=build /app/build/hotel_booking_hw06_consumer_userver /service/hotel_booking_hw06_consumer_userver
COPY static_config_api.yaml /service/static_config_api.yaml
COPY static_config_consumer.yaml /service/static_config_consumer.yaml
COPY secure_data.json /service/secure_data.json
EXPOSE 8080
