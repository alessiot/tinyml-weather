//
//  ScanView.swift
//  iosWeather
//
//  Created by Alessio Tamburro on 3/4/24.
//

import SwiftUI
import CoreBluetooth

struct ScanView: View {
    @ObservedObject var modelView: ScanModelView
    
    @State var shouldShowDetail = false
    @State var peripheralList = [Peripheral]()
    @State var isScanButtonEnabled = false
    
    var body: some View {
        VStack{
            List(peripheralList, id: \.id) { peripheral  in
                Text("\(peripheral.name ?? "N/A")")
                    .font(.headline)
                    .foregroundColor(.blue)
                    .onTapGesture {
                        modelView.connect(to: peripheral)
                    }
            }
            .listStyle(.plain)
            Spacer()
            Button {
                modelView.scan()
            } label: {
                Text("Scan")
                    .frame(maxWidth: .infinity)
            }
            .disabled(!isScanButtonEnabled)
            .buttonStyle(.borderedProminent)
            .padding(.horizontal)
        }
        .onReceive(modelView.$state) { state in
            switch state {
            case .connected:
                shouldShowDetail = true
            case .scan(let list):
                print("Scanning")
                peripheralList = list
                print("Received peripherals: \(peripheralList)")
            case .ready:
                isScanButtonEnabled = true
            default:
                print("Not handled \(state)")
            }
        }
        .navigationTitle("Available Sensors")
        .navigationDestination(isPresented: $shouldShowDetail) {
            if case let .connected(peripheral) = modelView.state  {
                let modelView = ConnectModelView(useCase: PeripheralUseCase(),
                    connectedPeripheral:peripheral)
                ConnectView(modelView: modelView)
            }
        }
        .onAppear {
            modelView.disconnectIfConnected()
        }
    }
}

// can we remove this?
struct ScanAndConnectView_Previews: PreviewProvider {
    
    final class FakeUseCase: CentralManagerUseCaseProtocol {
        
        var onPeripheralDiscovery: ((Peripheral) -> Void)?
        
        var onCentralState: ((CBManagerState) -> Void)?
        
        var onConnection: ((Peripheral) -> Void)?
        
        var onDisconnection: ((Peripheral) -> Void)?
        
        func start() {
            print("Central manager started")
            onCentralState?(.poweredOn)
        }
        func scan(for services: [CBUUID]) {
            DispatchQueue.main.asyncAfter(deadline: .now() + .seconds(1)) {
                self.onPeripheralDiscovery?(Peripheral(name: "inoBoard_1"))
                self.onPeripheralDiscovery?(Peripheral(name: "inoBoard_2"))
            }
        }
         
        func connect(to peripheral: Peripheral) {
            print("Connecting")
            onConnection?(.init(name: "inoBoard_1"))
        }
        
        func disconnect(from peripheral: Peripheral) {
            onDisconnection?(.init(name: "inoBoard_1"))
        }
        
    }
    
    static var modelView = {
        ScanModelView(useCase: FakeUseCase())
    }()
    
    static var previews: some View {
        NavigationStack {
            ScanView(modelView: modelView)
        }
    }
}
