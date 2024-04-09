//
//  ConnectView.swift
//  iosWeather
//
//  Created by Alessio Tamburro on 3/4/24.
//

import SwiftUI
import UIKit

struct ConnectView: View {
    
    @ObservedObject var modelView: ConnectModelView
    
    @Environment(\.dismiss) var dismiss
    
    @State var isToggleOn: Bool = false
    @State var isPeripheralReady: Bool = false
    @State var lastTemperature: Float = 999
    @State var lastHumidity: Float = 999
    @State var lastWeather: UInt8 = 2

    var body: some View {
        VStack {
            Text(modelView.connectedPeripheral.name ?? "Unknown")
                .font(.title)
            ZStack {
                CardView()
                VStack {
                    Text((lastWeather == 2) ? "WAIT" : (lastWeather == 0) ? "No rain" : "Rain")
                        .foregroundColor((lastWeather == 2) ? .blue : (lastWeather == 0) ? .green : .red)
                        .font(.largeTitle)
                    HStack {
                        Spacer()
                            .frame(alignment: .trailing)
                        .disabled(!isPeripheralReady)
                        .buttonStyle(.borderedProminent)
                        Spacer()
                            .frame(alignment: .trailing)

                    }
                }
            }
            ZStack {
                CardView()
                VStack {
                    Text("Temperature")
                            .font(.subheadline)
                            .foregroundColor(.blue)
                    Text((lastTemperature == 999) ? "WAIT" : "\(String(format: "%.1f", lastTemperature)) °C")
                        .foregroundColor((lastTemperature == 999) ? .blue : .white)
                        .font(.largeTitle)
                    HStack {
                        Spacer()
                            .frame(alignment: .trailing)
                        .disabled(!isPeripheralReady)
                        .buttonStyle(.borderedProminent)
                        Spacer()
                            .frame(alignment: .trailing)

                    }
                }
            }.padding()
            ZStack {
                CardView()
                VStack {
                    Text("Humidity")
                            .font(.subheadline)
                            .foregroundColor(.blue)
                    Text((lastHumidity == 999) ? "WAIT" : "\(String(format: "%.1f", lastHumidity)) %")
                        .foregroundColor((lastHumidity == 999) ? .blue : .white)
                        .font(.largeTitle)
                    HStack {
                        Spacer()
                            .frame(alignment: .trailing)
                        .disabled(!isPeripheralReady)
                        .buttonStyle(.borderedProminent)
                        Spacer()
                            .frame(alignment: .trailing)

                    }
                }
            }
            ZStack {
                //CardView()
                VStack {
                    HStack {
                        Spacer()
                            .frame(alignment: .trailing)
                        Toggle("Auto", isOn: $isToggleOn)
                            .disabled(!isPeripheralReady)
                        Button("Read") {
                            modelView.readSensor()
                        }
                        .disabled(!isPeripheralReady)
                        .buttonStyle(.borderedProminent)
                        Spacer()
                            .frame(alignment: .trailing)

                    }
                }
            }
            Spacer()
                .frame(maxHeight:.infinity)
            Button {
                dismiss()
            } label: {
                Text("Disconnect")
                    .frame(maxWidth: .infinity)
            }
            .buttonStyle(.borderedProminent)
            .padding(.horizontal)
        }
        .onChange(of: isToggleOn) { newValue in
            if newValue == true {
                modelView.startNotifySensor()
            } else {
                modelView.stopNotifySensor()
            }
        }
        .onReceive(modelView.$state) { state in
            switch state {
            case .ready:
                isPeripheralReady = true
            case let .sensorReading(temp, hum, mdlout):
                lastTemperature = temp
                lastHumidity = hum
                lastWeather = mdlout
            default:
                print("Not handled")
            }
        }

    }
}

struct PeripheralView_Previews: PreviewProvider {
    
    final class FakeUseCase: PeripheralUseCaseProtocol {
        
        var peripheral: Peripheral?
        
        var onSensorReading: SensorReadingCallback?
        var onPeripheralReady: (() -> Void)?
        var onError: ((Error) -> Void)?

        func readSensor() {
            onSensorReading?(999, 999, 2) 
        }
        
        func notifySensor(_ isOn: Bool) {}

    }
    
    static var modelView = {
        ConnectModelView(useCase: FakeUseCase(),
                            connectedPeripheral: .init(name: "iosWeather"))
    }()
    
    
    static var previews: some View {
        ConnectView(modelView: modelView, isPeripheralReady: true)
    }
}

struct CardView: View {
  var body: some View {
    RoundedRectangle(cornerRadius: 16, style: .continuous)
      .shadow(color: Color(white: 0.5, opacity: 0.2), radius: 6)
      .foregroundColor(.init(uiColor: .secondarySystemBackground))
  }
}
